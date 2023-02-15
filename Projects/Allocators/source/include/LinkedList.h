#pragma once

template<typename T>
struct Link
{
    T data;
    Link<T>* next;
};

template<typename T>
struct Pool
{
    Link<T>* head;
    Pool* next;
};

template<typename T>
static Pool<T>* Allocate( int capacity )
{
    Pool<T>* newPool = ( Pool<T>* )malloc( sizeof( Pool<T> ) );
    if( !newPool )
    {
        fprintf( stderr, "ERROR: No memory available for pool!" );
        exit( -1 );
    }
    newPool->next = nullptr;

    newPool->head = ( Link<T>* )malloc( sizeof( Link<T> ) * capacity );
    if( newPool->head )
    {
        for( int i = 0; i < capacity - 1; ++i )
        {
            newPool->head[i].data = {};
            newPool->head[i].next = &newPool->head[i + 1];
        }
        newPool->head[capacity - 1].data = {};
        newPool->head[capacity - 1].next = nullptr;
    }
    else
    {
        fprintf( stderr, "ERROR: No memory available for pool with capacity %i!", capacity );
        exit( -1 );
    }

    return newPool;
}

template<typename T>
static int Deallocate( Pool<T>* pool )
{
    if( !pool )
    {
        fprintf( stderr, "ERROR: Attempting to deallocate a null pool!" );
        exit( -1 );
    }

    Pool<T>* nextPool = pool->next;

    free( pool->head );
    pool->head = nullptr;

    free( pool );
    pool = nullptr;

    int poolCount = 1;
    while( nextPool )
    {
        pool = nextPool;
        nextPool = pool->next;

        free( pool->head );
        pool->head = nullptr;

        free( pool );
        pool = nullptr;

        poolCount += 1;
    }

    return poolCount;
}

template<typename T>
class LinkedList
{
    public:

    Link<T>* first = nullptr;
    Link<T>* last = nullptr;

    LinkedList( int poolCapacity = 4096 )
    {
        _capacity = poolCapacity > 2 ? poolCapacity : 2;
        _pool = Allocate<T>( _capacity );
        first = _pool->head;
        last = first;
    }

    ~LinkedList()
    {
        fprintf( stdout, "LinkedList released %i pool(s).\n", Deallocate( _pool ) );
    }

    Link<T>* Append( T data )
    {
        // if there aren't any links after 'last' pointer, allocate a new pool
        if( last->next == nullptr )
        {
            Pool<T>* pool = _pool;
            while( pool->next )
                pool = pool->next;

            pool->next = Allocate<T>( _capacity );
            last->next = pool->next->head;
        }

        // store data in first empty link (tracked with 'last')
        Link<T>* link = last;
        link->data = data;

        // move 'last' pointer to next empty link
        last = last->next;

        return link;
    }

    bool Remove( Link<T>* link )
    {
        Link<T>* prev = first;
        Link<T>* node = first->next;

        if( !link )
        {
            fprintf( stderr, "ERROR: Attempting to remove a null link!" );
            exit( -1 );
        }

        while( node )
        {
            if( node == link )
            {
                // link previous node with node proceeding this one
                prev->next = node->next;

                // empty this link's data
                node->data = {};

                // point this node's next link to node proceeding 'last' link
                node->next = last->next;

                // point 'last' link to this node, appending it back into the list
                last->next = node;

                return true;
            }

            prev = node;
            node = node->next;
        }

        return false;
    }

    private:

    Pool<T>* _pool;
    int _capacity = 4096;
};
