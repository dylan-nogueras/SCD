#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;

const int num_clientes = 3; // número de clientes
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
// Clase para monitor Barberia, semántica SU

class Barberia : public HoareMonitor
{
    private:
        CondVar                         // colas condición:
            barbero,                    // cola donde espera el barbero
            silla,                      // cola de la silla en la que se pela a los clientes
            sala_espera;                // cola donde esperan los clientes

    public:                                 // constructor y métodos públicos
        Barberia(  ) ;                      // constructor
        void cortarPelo(int num_cliente);   // cortar el pelo (cliente)
        void siguienteCliente();            // llamar al siguiente cliente (barbero)
        void finCliente();                  // avisar al cliente de que ya ha terminado (barbero)
} ;
// -----------------------------------------------------------------------------

Barberia::Barberia(  )
{
    barbero = newCondVar();
    sala_espera = newCondVar();
    silla = newCondVar();
}
// -----------------------------------------------------------------------------
// Función llamada por el cliente para que el barbero le corte el pelo

void Barberia::cortarPelo(int num_cliente)
{
    // El cliente espera a que el barbero no esté ocupado
    if (!silla.empty() || !sala_espera.empty()){
        cout << "El cliente " << num_cliente << " espera en la sala. " << endl;
        sala_espera.wait();
    }
    // El cliente despierta al barbero
    if (!barbero.empty()){
        cout << "El cliente " << num_cliente << " despierta al barbero. " << endl;
        barbero.signal();
    }

    cout << "El cliente " << num_cliente << " pasa y se sienta para ser pelado. " << endl;
    silla.wait();  
}
// -----------------------------------------------------------------------------
// Función llamada por el barbero para avisar al siguiente cliente

void Barberia::siguienteCliente()
{
    if (sala_espera.empty() && silla.empty()){
        cout << "El barbero se duerme. " << endl;
        barbero.wait();
    }

    if (silla.empty()){
        cout << "Que pase el siguiente, por favor. Hay " << sala_espera.get_nwt() << " clientes en espera. " << endl;
        sala_espera.signal();
    }
    
}
// -----------------------------------------------------------------------------
// Función llamada por el barbero para avisar al cliente de que ha terminado de pelarlo

void Barberia::finCliente(){
    // Avisa al cliente de que ha terminado de pelarlo
    silla.signal();
}

// *****************************************************************************

// Función que simula el corte de pelo, con una espera aleatoria

void cortarPeloACliente()
{
    // calcular milisegundos aleatorios de duración de la acción de pelar)
    chrono::milliseconds duracion_pelar( aleatorio<20,200>() );
    mtx.lock();
    cout << "El barbero está cortando el pelo. Tarda " << duracion_pelar.count() << " milisegundos. " << endl;
    mtx.unlock();
    this_thread::sleep_for(duracion_pelar);
    
    mtx.lock();
    cout << "El barbero ha terminado de pelar. " << endl;
    mtx.unlock();
}

//-------------------------------------------------------------------------
// Función que ejecuta la hebra del barbero

void funcion_hebra_barbero( MRef<Barberia> monitor )
{
    while(true){
        monitor->siguienteCliente();
        cortarPeloACliente();
        monitor->finCliente();
    }

}

//-------------------------------------------------------------------------
// Función que simula la acción de esperar fuera de la barbería

void esperarFueraBarberia( int num_cliente)
{

    // calcular milisegundos aleatorios de duración de la acción de la espera)
    chrono::milliseconds duracion_espera( aleatorio<20,200>() );
    mtx.lock();
    cout << "El cliente " << num_cliente << " espera fuera " << duracion_espera.count() << " milisegundos. " << endl;
    mtx.unlock();
    // espera bloqueada un tiempo igual a ''duracion_espera' milisegundos
    this_thread::sleep_for(duracion_espera);

    // informa de que ha terminado de esperar
    mtx.lock();
    cout << "El cliente " << num_cliente << " termina de esperar fuera y entra de nuevo a la barbería." << endl;
    mtx.unlock();
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra del cliente
void funcion_hebra_cliente(MRef<Barberia> monitor, int num_cliente)
{
    while(true){
        monitor->cortarPelo(num_cliente);
        esperarFueraBarberia(num_cliente);
    }
}

//----------------------------------------------------------------------

int main()
{
    cout << "--------------------------------------------------------" << endl
    << "Problema de la barbería con monitores SU." << endl
    << "--------------------------------------------------------" << endl
    << flush ;

    MRef<Barberia> monitor = Create<Barberia>();

    thread hebra_barbero;
    thread hebra_cliente[num_clientes];

    hebra_barbero = thread(funcion_hebra_barbero,monitor);
    for (int i=0; i < num_clientes; i++)
        hebra_cliente[i] = thread(funcion_hebra_cliente,monitor,i);
  
    hebra_barbero.join() ;
    for (int i=0; i < num_clientes; i++)
        hebra_cliente[i].join();

    cout << "Fin" << endl;
}
