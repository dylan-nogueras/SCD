// -----------------------------------------------------------------------------
// Aurelia María Nogueras Lara 77552127Z
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// 1.¿Cuál es el mínimo tiempo de espera que queda al final de las
// iteraciones del ciclo secundario con tu solución?
// 
// El mínimo tiempo de espera de la primera iteración del ciclo secundario sería de 50 milisegundos.
// El mínimo tiempo de espera de la segunda iteración del ciclo secundario sería de 10 milisegundos.
// El mínimo tiempo de espera de la tercera iteración del ciclo secundario sería de 50 milisegundos.
// El mínimo tiempo de espera de la cuarta iteración del ciclo secundario sería de 250 milisegundos.
//
// 2.¿Sería planificable si la tarea D tuviese un tiempo cómputo de
// 250 ms?
//
// Sí, pero de esta manera, la tercera iteración del ciclo secundario ocuparía 500 milisegundos, con lo que no 
// tendría que producirse ninguna espera.
//
// Archivo: ejercicio2.cpp
// Implementación del nuevo ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500  100
//   B  500  150
//   C  1000 200
//   D  2000 240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D    | A B C   | A B    |
//  *---------*----------*---------*--------*
//
//
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario
   const milliseconds Ts( 500 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();

   // in_final = instante final dentro del ciclo secundario
   time_point<steady_clock> in_final;

   // duracion = variable para medir el retraso del tiempo de finalización del ciclo secundario
   steady_clock::duration duracion;

   while( true ) // ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD();           break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB();                     break ;
         }

         // calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts ;

         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );

         // calcular el retraso entre el final producido del ciclo secundario y el final esperado
         in_final = steady_clock::now();
         duracion = in_final - ini_sec;

         cout << "Retraso de ciclo " << i << " de " << seconds_f(duracion).count() << " segundos." << endl ;
         if (milliseconds_f(duracion).count() > 20){
             cout << "ERROR: Se ha excedido el retraso permitido. Se aborta la ejecución." << endl;
             exit(1);
         }

      }
   }
}
