#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"
#include <atomic>

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_consumidores = 5;
const int num_items = 60 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos
Semaphore ocupadas = 0, libres = tam_vec;
int buffer[tam_vec], primera_libre=0, primera_ocupada=0; //FIFO
mutex mtx,mtx2;
atomic<int> contador_atom;

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
  static int contador = 0 ;
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
  mtx.lock();
  cout << "producido: " << contador << endl << flush ;
  mtx.unlock();
  cont_prod[contador] ++ ;
  return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, int num_consumidor)
{
  assert( dato < num_items );
  cont_cons[dato] ++ ;
	contador_atom++;
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
  mtx.lock();
  cout << "consumido: " << dato << " por consumidor " << num_consumidor << endl ;
  mtx.unlock();
}


//----------------------------------------------------------------------

void test_contadores()
{
  bool ok = true ;
  cout << "comprobando contadores ...." ;
  for( unsigned i = 0 ; i < num_items ; i++ )
  {  if ( cont_prod[i] != 1 )
     {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
       ok = false ;
     }
     if ( cont_cons[i] != 1 )
     {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
        ok = false ;
     }
  }
  if (ok)
     cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{
  for( unsigned i = 0 ; i < num_items ; i++ )
  {
     int dato = producir_dato() ;
     sem_wait(libres);
     //insertar dato en buffer
     buffer[primera_libre] = dato;
     primera_libre++;
     if(primera_libre >= tam_vec)
       primera_libre=0;
     sem_signal(ocupadas);
  }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(int num_consumidor)
{
  for(unsigned i = 0 ; i < num_items/num_consumidores; i++)
  {
     int dato ;
     sem_wait(ocupadas);
     //extraer dato de buffer
		 mtx2.lock();
     dato = buffer[primera_ocupada];
     primera_ocupada++;
     if(primera_ocupada >= tam_vec)
       primera_ocupada=0;
		 mtx2.unlock();
     sem_signal(libres);
     consumir_dato(dato,num_consumidor) ;
   }
}
//----------------------------------------------------------------------

int main()
{
  cout << "--------------------------------------------------------" << endl
       << "Problema de los productores-consumidores (solución FIFO)." << endl
       << "--------------------------------------------------------" << endl
       << flush ;
	
	thread hebra_productora, hebra_consumidora[num_consumidores];
  
	hebra_productora = thread(funcion_hebra_productora );
  for (int i=0; i<num_consumidores; i++)
		hebra_consumidora[i] = thread(funcion_hebra_consumidora,i);

  hebra_productora.join() ;
  for (int i=0; i<num_consumidores; i++)
		hebra_consumidora[i].join();


  test_contadores();

	cout << "El número de items que se han consumido en total es " << contador_atom << endl;
  cout << "Fin" << endl;
}
