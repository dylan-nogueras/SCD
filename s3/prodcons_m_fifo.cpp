// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
// Aurelia María Nogueras Lara
//
// Productor consumidor con múltiples prod-cons FIFO
// mpicxx -std=c++11 -o prodcons prodcons_m_fifo.cpp
// mpirun -np 10 ./prodcons

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
    num_prod = 4,
    num_cons = 5,
    num_procesos_esperado = 10,
    num_items             = 20,
    tam_vector            = 10;

// Inicialización de los identificadores del buffer
const int id_buffer = num_prod;

// Etiquetas
const int etiqueta_consumidor = 2,
          etiqueta_productor = 1,
          etiqueta_buffer = 0;

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
// producir produce los números en secuencia (1,2,3,....)
// y lleva espera aleatoria
int producir(int num_p)
{
    static int contador = 0 ;  
    sleep_for( milliseconds( aleatorio<10,100>()) );
    contador++ ;
    cout << "Productor " << num_p << " ha producido valor " << contador << endl << flush;
    return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int num_p)
{
    for ( unsigned int i= 0 ; i < num_items/num_prod ; i++ )
    {
        // producir valor
        int valor_prod = producir(num_p);
        // enviar valor
        cout << "Productor " << num_p << " va a enviar valor " << valor_prod << endl << flush;
        MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiqueta_productor, MPI_COMM_WORLD );
    }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons, int num_c )
{
    // espera bloqueada
    sleep_for( milliseconds( aleatorio<110,200>()) );
    cout << "Consumidor " << num_c << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int num_c)
{
    int     peticion,
            valor_rec = 1 ;
    MPI_Status  estado ;

    for( unsigned int i=0 ; i < num_items/num_cons; i++ )
    {
        MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiqueta_consumidor, MPI_COMM_WORLD);
        MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiqueta_buffer, MPI_COMM_WORLD,&estado );
        cout << "Consumidor " << num_c << " ha recibido valor " << valor_rec << endl << flush ;
        consumir(valor_rec,num_c);
    }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
    int       buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              peticion,                // petición realizada por el consumidor 
              opcion;                  // opción entre productor o consumidor
    MPI_Status estado ;                // metadatos del mensaje recibido

    for( unsigned int i=0 ; i < num_items*2 ; i++ )
    {
        // 1. determinar si puede enviar solo prod., solo cons, o todos

        if ( num_celdas_ocupadas == 0 )               // si buffer vacío
            opcion = 0;                               // $~~~$ solo prod.
        else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
            opcion = 1;                               // $~~~$ solo cons.
        else {                                        // si no vacío ni lleno
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);     // $~~~$ espera a que cualquiera esté disponible
      
            if (estado.MPI_TAG == etiqueta_productor)
                opcion = 0;
            else
                opcion = 1;
        }

        // 2. recibir un mensaje del emisor o emisores aceptables y procesar el mensaje recibido

        switch(opcion) // leer emisor del mensaje en metadatos
        {
            case 0: // si ha sido el productor: insertar en buffer
                MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_productor, MPI_COMM_WORLD, &estado );
                buffer[primera_libre] = valor ;
                primera_libre = (primera_libre+1) % tam_vector ;
                num_celdas_ocupadas++ ;
                cout << "Buffer ha recibido valor " << valor << " del productor " << estado.MPI_SOURCE << endl ;
                break;

            case 1: // si ha sido el consumidor: extraer y enviarle
                MPI_Recv( &peticion, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_consumidor, MPI_COMM_WORLD, &estado );
                valor = buffer[primera_ocupada] ;
                primera_ocupada = (primera_ocupada+1) % tam_vector ;
                num_celdas_ocupadas-- ;
                cout << "Buffer va a enviar valor " << valor << " al consumidor " << estado.MPI_SOURCE << endl ;
                MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiqueta_buffer, MPI_COMM_WORLD);
                break;
        }
    }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
    int id_propio, num_procesos_actual;

    // inicializar MPI, leer identif. de proceso y número de procesos
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
    MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

    if ( num_procesos_esperado == num_procesos_actual )
    {
        // ejecutar la operación apropiada a 'id_propio'
        if ( id_propio < num_prod)
            funcion_productor(id_propio);
        else if ( id_propio == id_buffer )
            funcion_buffer();
        else
            funcion_consumidor(id_propio);
    }
    else
    {
        if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
        { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
        }
    }

    // al terminar el proceso, finalizar MPI
    MPI_Finalize( );
    return 0;
}
