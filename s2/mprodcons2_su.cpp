// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: mprodcons2_su.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Opcion FIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir
const int num_prod = 4; // número de productores   
const int num_cons = 5; // número de consumidores
mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
    contador_p[num_prod], // contador de items producidos por cada productor
   contador_c[num_cons], // contador de items consumidos por cada consumidor
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos

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

int producir_dato(unsigned num_hebra)
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << " por hebra " << num_hebra << endl << flush ;
   contador_p[num_hebra]++;
   cout << "hebra " << num_hebra << " ha producido " << contador_p[num_hebra] << " items " << endl;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, unsigned num_hebra )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << " por hebra " << num_hebra << endl ;
   contador_c[num_hebra]++;
   cout << "hebra " << num_hebra << " ha consumido " << contador_c[num_hebra] << " items " << endl;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
   for(unsigned i = 0; i < num_prod; i++)
    contador_p[i] = 0;
    for (unsigned i = 0; i < num_cons; i++)
        contador_c[i] = 0;
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SU, múltiples productores y consumidores

class ProdConsSU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   celdas_ocupadas,         // número de celdas ocupadas
   primera_ocupada,         // índice de celda de la próxima extracción
   primera_libre ;          //  indice de celda de la próxima inserción
    CondVar                 // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsSU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsSU::ProdConsSU(  )
{
   primera_libre = 0 ;
   primera_ocupada = 0;
   celdas_ocupadas = 0;
   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( celdas_ocupadas == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < celdas_ocupadas  );
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada++ ;
   celdas_ocupadas--;
   primera_ocupada %= num_celdas_total;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsSU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( celdas_ocupadas == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( celdas_ocupadas < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;
   celdas_ocupadas++;
   primera_libre %= num_celdas_total;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsSU> monitor, unsigned num_hebra )
{
   for( unsigned i = 0 ; i < num_items/num_prod ; i++ )
   {
      int valor = producir_dato(num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsSU> monitor, unsigned num_hebra )
{
   for( unsigned i = 0 ; i < num_items/num_cons ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor, num_hebra ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (múltiples prod/cons, Monitor SU, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsSU> monitor = Create<ProdConsSU>();

   thread hebra_productora [num_prod], hebra_consumidora [num_cons];

    for (unsigned i = 0; i < num_prod; i++)
       hebra_productora[i] = thread ( funcion_hebra_productora, monitor, i);

    for (unsigned i = 0; i < num_cons; i++)
       hebra_consumidora[i] = thread ( funcion_hebra_consumidora, monitor, i);


   for (unsigned i = 0; i < num_prod; i++)
       hebra_productora[i].join();

    for (unsigned i = 0; i < num_cons; i++)
       hebra_consumidora[i].join();


   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
