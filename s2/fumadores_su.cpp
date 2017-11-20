// g++ -std=c++11 -pthread -o fum fumadores_su.cpp HoareMonitor.cpp
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;

mutex mtx;

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

// *****************************************************************************
// Clase para monitor Estanco, semántica SU

class Estanco : public HoareMonitor
{
    private:
        bool hay_ing;
        int ingrediente_comun;
        CondVar                 // colas condicion:
            mostr_vacio,        // cola donde espera el estanquero
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
    hay_ing = false;
    ingrediente_comun = -1;
}
// -----------------------------------------------------------------------------
// Función llamada por el fumador para obtener su ingrediente

void Estanco::obtenerIngrediente(int num_ingrediente)
{
    // El fumador espera a que su ingrediente esté disponible y lo retira
    if (ingrediente_comun != num_ingrediente || !hay_ing)
        ingr_disp[num_ingrediente].wait();
    hay_ing = false;
    cout << "El fumador " << num_ingrediente << " ha retirado el ingrediente. " << endl;
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
    mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
    << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mtx.unlock();
    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for( duracion_fumar );

    // informa de que ha terminado de fumar
    mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mtx.unlock();
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

int main()
{
    cout << "--------------------------------------------------------" << endl
    << "Problema de los fumadores con monitores SU." << endl
    << "--------------------------------------------------------" << endl
    << flush ;

    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_estanquero;
    thread hebra_fumador[3];

    hebra_estanquero = thread(funcion_hebra_estanquero,monitor);
    hebra_fumador[0] = thread(funcion_hebra_fumador,monitor,0);
    hebra_fumador[1] = thread(funcion_hebra_fumador,monitor,1);
    hebra_fumador[2] = thread(funcion_hebra_fumador,monitor,2);


    hebra_estanquero.join() ;
    hebra_fumador[0].join() ;
    hebra_fumador[1].join() ;
    hebra_fumador[2].join() ;

    cout << "Fin" << endl;
}
