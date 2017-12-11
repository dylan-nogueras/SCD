// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
// Aurelia María Nogueras Lara
//
// Filósofos con camarero (versión correcta)
// mpicxx -std=c++11 -o filosofos filosofos.cpp
// mpirun -np 10 ./filosofos
// -----------------------------------------------------------------------------

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
    num_filosofos = 5 ,
    num_procesos  = 2*num_filosofos,
    etiqueta_coger = 0,
    etiqueta_soltar = 1;


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

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
    int id_ten_izq = (id+1)              % num_procesos, //id. tenedor izq.
        id_ten_der = (id+num_procesos-1) % num_procesos; //id. tenedor der.

    while ( true )
    {
        // Esta solución evita el interbloqueo
        if (id % 4 == 0) // Si se trata de un filósofo divisible por cuatro
        {
            // Coge primero el tenedor izquierdo y después el derecho
            cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
            // solicitar tenedor izquierdo
            MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiqueta_coger, MPI_COMM_WORLD);
        
            cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
            // solicitar tenedor derecho
            MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiqueta_coger, MPI_COMM_WORLD);
        }else // Si no es divisible por 4, coge primero el tenedor derecho y después el izquierdo
        {
            cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
            // solicitar tenedor derecho
            MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiqueta_coger, MPI_COMM_WORLD);

            cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
            // solicitar tenedor izquierdo
            MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiqueta_coger, MPI_COMM_WORLD);
        }
            
        cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
        sleep_for( milliseconds( aleatorio<10,100>() ) );

        // El orden en el que se sueltan permanece como en la solución anterior

        cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
        // soltar el tenedor izquierdo
        MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiqueta_soltar, MPI_COMM_WORLD);

        cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
        // soltar el tenedor derecho
        MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiqueta_soltar, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " comienza a pensar" << endl;
        sleep_for( milliseconds( aleatorio<10,100>() ) );
    }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
    int valor, id_filosofo ;  // valor recibido, identificador del filósofo
    MPI_Status estado ;       // metadatos de las dos recepciones

    while ( true )
    {
        // recibir petición de cualquier filósofo
        // guardar en 'id_filosofo' el id. del emisor
        MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_coger, MPI_COMM_WORLD,&estado );
        id_filosofo = valor;
        
        cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

        // recibir liberación de filósofo 'id_filosofo'
        MPI_Recv ( &id_filosofo, 1, MPI_INT, id_filosofo, etiqueta_soltar, MPI_COMM_WORLD,&estado );
        cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
    }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
    int id_propio, num_procesos_actual ;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
    MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


    if ( num_procesos == num_procesos_actual )
    {
        // ejecutar la función correspondiente a 'id_propio'
        if ( id_propio % 2 == 0 )          // si es par
            funcion_filosofos( id_propio ); //   es un filósofo
        else                               // si es impar
            funcion_tenedores( id_propio ); //   es un tenedor
    }
    else
    {
        if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
        { cout << "el número de procesos esperados es:    " << num_procesos << endl
                << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                << "(programa abortado)" << endl ;
        }
    }

    MPI_Finalize( );
    return 0;
}

// ---------------------------------------------------------------------
