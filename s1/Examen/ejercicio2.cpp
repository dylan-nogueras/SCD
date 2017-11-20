#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"
#include <atomic>


using namespace std ;
using namespace SEM ;






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

//----------------------------------------------------------------------
// Variables compartidas
Semaphore mostr_vacio = 1;
Semaphore ingr_disp [3] = {0,0,0};
Semaphore compartido = 0;
atomic<int> contador_atom;

// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
  int i;

  // calcular milisegundos aleatorios de duración de la acción de repartir)
  chrono::milliseconds duracion_repartir( aleatorio<20,200>() );

  while(true){
    i = aleatorio<0,2>();
    this_thread::sleep_for(duracion_repartir);
    sem_wait(mostr_vacio);
    cout << "El estanquero ha repartido el ingrediente: " << i << endl;
    sem_signal(ingr_disp[i]);
  }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

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
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
  while(true){
    sem_wait(ingr_disp[num_fumador]);
    cout << "El fumador " << num_fumador << " ha retirado el ingrediente. " << endl;
    sem_signal(mostr_vacio);
    fumar(num_fumador);
		contador_atom++;
		sem_signal(compartido);
  }
}

//----------------------------------------------------------------------
// función de la hebra contadora
void funcion_hebra_contadora()
{
	while(true){
		sem_wait(compartido);
		cout << "Ejecución de la hebra contadora. " << endl;
		cout << "Se ha fumado un total de " << contador_atom << " veces. " << endl;
	}
}

//----------------------------------------------------------------------

int main()
{
  cout << "--------------------------------------------------------" << endl
  << "Problema de los fumadores." << endl
  << "--------------------------------------------------------" << endl
  << flush ;

	thread hebra_contadora;
  thread hebra_estanquero;
  thread hebra_fumador[3];

  hebra_estanquero = thread(funcion_hebra_estanquero);
  hebra_fumador[0] = thread(funcion_hebra_fumador,0);
  hebra_fumador[1] = thread(funcion_hebra_fumador,1);
  hebra_fumador[2] = thread(funcion_hebra_fumador,2);
	hebra_contadora = thread(funcion_hebra_contadora);


  hebra_estanquero.join() ;
  hebra_fumador[0].join() ;
  hebra_fumador[1].join() ;
  hebra_fumador[2].join() ;
	hebra_contadora.join();

  cout << "Fin" << endl;
}
