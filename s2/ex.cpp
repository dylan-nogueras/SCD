// Examen del otro grupo
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
    static default_random_engine generador( (random_device())() );
    static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
    return distribucion_uniforme( generador );
}

int contador[]={0,0,0};

// *****************************************************************************
// Clase para monitor Estanco, semántica SU

class Estanco : public HoareMonitor
{
    private:
        bool hay_ing;
        int ingrediente_comun;
        int ingr_reunido_doble[3];
        int ingrediente_anterior;
        CondVar                 // colas condicion:
            mostr_vacio,        // cola donde espera el estanquero
            fumar[3],
            ingr_disp[3] ;      // cola donde esperan los fumadores

    public:                    // constructor y métodos públicos
        Estanco(  ) ;           // constructor
        void obtenerIngrediente(int num_ingrediente);   // obtener un ingrediente (fumador)
        void ponerIngrediente(int num_ingrediente);     // insertar un ingrediente (estanquero)
        void esperarRecogidaIngrediente();              // esperar la recogida del ingrediente (estanquero)
} ;
// -----------------------------------------------------------------------------

Estanco::Estanco(  )
{
    mostr_vacio = newCondVar();
    ingr_disp[0] = newCondVar();
    ingr_disp[1] = newCondVar();
    ingr_disp[2] = newCondVar();
    fumar[0] = newCondVar();
    fumar[1] = newCondVar();
    fumar[2] = newCondVar();
    hay_ing = false;
    ingrediente_comun = -1;
    ingrediente_anterior = -1;
    for (int i=0; i<=2; i++)
        ingr_reunido_doble[i]=0;
}
// -----------------------------------------------------------------------------
// Función llamada por el fumador para obtener su ingrediente

void Estanco::obtenerIngrediente(int num_ingrediente)
{
    // El fumador espera a que su ingrediente esté disponible y lo retira
    
    if (ingrediente_comun != num_ingrediente || !hay_ing)
        ingr_disp[num_ingrediente].wait();
    
    hay_ing = false;
    mostr_vacio.signal();
    fumar[num_ingrediente].wait();
    cout << "El fumador " << num_ingrediente << " ha retirado los ingredientes. " << endl;
    contador[num_ingrediente]++;
    hay_ing = false;
    mostr_vacio.signal();
}
// -----------------------------------------------------------------------------
// Función llamada por el estanquero para poner un ingrediente en el mostrador

void Estanco::ponerIngrediente( int num_ingrediente)
{
    // El estanquero pone el ingrediente i y espera a que sea recogido
    ingrediente_comun = num_ingrediente;
    hay_ing = true;
    cout << "El estanquero ha repartido el ingrediente: " << num_ingrediente << endl;
    if (num_ingrediente == ingrediente_anterior)
        cout << "Hey, estanquero, has repartido dos veces el mismo ingrediente." << endl;
    ingrediente_anterior = num_ingrediente;
    ingr_reunido_doble[num_ingrediente] %= 2;
    ++ingr_reunido_doble[num_ingrediente];
    cout << ingr_reunido_doble[num_ingrediente] << endl;
    if (ingr_reunido_doble[num_ingrediente] == 2)
        fumar[num_ingrediente].signal();
    else
        ingr_disp[num_ingrediente].signal();
}
// -----------------------------------------------------------------------------
// Función llamada por el estanquero para esperar a que el ingrediente sea recogido

void Estanco::esperarRecogidaIngrediente(){
    // Espera a que el mostrador esté libre
    if (hay_ing)
        mostr_vacio.wait();
}

// *****************************************************************************

// Función que simula la producción del ingrediente, con un tiempo aleatorio de espera

int producirIngrediente()
{
    int i;
    // calcular milisegundos aleatorios de duración de la acción de repartir)
    chrono::milliseconds duracion_repartir( aleatorio<20,200>() );
    i = aleatorio<0,2>();
    this_thread::sleep_for(duracion_repartir);
    return i;
}

//-------------------------------------------------------------------------
// Función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
    int i;
    while(true){
        i = producirIngrediente();
        monitor->ponerIngrediente(i);
        monitor->esperarRecogidaIngrediente();
  }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatorio de la hebra

void fumar( int num_fumador )
{

    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

    // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
    << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for( duracion_fumar );

    // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra del fumador
void funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador )
{
    while(true){
        monitor->obtenerIngrediente(num_fumador);
        fumar(num_fumador);
  }
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra contador
void funcion_hebra_contador()
{
    int maximo = 0;
    int fumador = -1;
    while(true){
        chrono::milliseconds duracion_espera( aleatorio<20,200>() );
        // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
        this_thread::sleep_for( duracion_espera );
        if(contador[1]>contador[0]){
            maximo = contador[1];
            fumador = 1;
        }else{ 
            maximo = contador[0];
            fumador = 0;
        }
        if(contador[2]>maximo){
            maximo = contador[2];
            fumador = 2;
        }
        cout << "El fumador más vicioso hasta el momento es el número " << fumador << " que ha fumado " << maximo << " veces." << endl;
  }
}

//----------------------------------------------------------------------

int main()
{
    cout << "--------------------------------------------------------" << endl
    << "Problema de los fumadores con monitores SU." << endl
    << "--------------------------------------------------------" << endl
    << flush ;

    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_estanquero;
    thread hebra_fumador[3];
    thread hebra_contador;

    hebra_estanquero = thread(funcion_hebra_estanquero,monitor);
    hebra_fumador[0] = thread(funcion_hebra_fumador,monitor,0);
    hebra_fumador[1] = thread(funcion_hebra_fumador,monitor,1);
    hebra_fumador[2] = thread(funcion_hebra_fumador,monitor,2);
    hebra_contador = thread(funcion_hebra_contador);


    hebra_estanquero.join() ;
    hebra_fumador[0].join() ;
    hebra_fumador[1].join() ;
    hebra_fumador[2].join() ;
    hebra_contador.join();

    cout << "Fin" << endl;
}
