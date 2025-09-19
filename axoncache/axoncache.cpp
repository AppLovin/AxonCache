// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// axoncache.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

#include "../AxonCache.cpp"

namespace py = pybind11;

extern "C"
{
#include "axoncache/capi/CacheReaderCApi.h"
#include "axoncache/capi/CacheWriterCApi.h"
}

struct NotFoundError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct UninitializedError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class Reader
{
  public:
    Reader() :
        handle_( NewCacheReaderHandle() )
    {
        if ( !handle_ )
        {
            throw std::runtime_error( "NewCacheReaderHandle returned null" );
        }
    }

    void update( const std::string & task_name,
                 const std::string & destination_folder,
                 const std::string & timestamp )
    {
        int ret = CacheReader_Initialize( handle_,
                                          task_name.c_str(),
                                          destination_folder.c_str(),
                                          timestamp.c_str(),
                                          0 );
        if ( ret != 0 )
        {
            throw std::runtime_error( "CacheReader_Initialize failed with code " + std::to_string( ret ) );
        }
        initialized_ = true;
    }

    bool contains_key( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int has = CacheReader_ContainsKey( handle_, k.data(), ( size_t )k.size() );
        return has != 0;
    }

    py::bytes get_key( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int exists = 0, size = 0;
        char * buf = CacheReader_GetKey( handle_, k.data(), ( size_t )k.size(), &exists, &size );
        struct Guard
        {
            char * p;
            ~Guard()
            {
                if ( p )
                {
                    free( p );
                }
            }
        } g{ buf };
        if ( exists == 0 )
        {
            throw NotFoundError( "key not found" );
        }
        if ( !buf && size > 0 )
        {
            throw std::runtime_error( "nil buffer with positive size" );
        }
        return py::bytes( buf, size );
    }

    std::string get_key_type( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int size = 0;
        char * buf = CacheReader_GetKeyType( handle_, k.data(), ( size_t )k.size(), &size );
        struct Guard
        {
            char * p;
            ~Guard()
            {
                if ( p )
                {
                    free( p );
                }
            }
        } g{ buf };
        if ( !buf )
        {
            throw NotFoundError( "key not found" );
        }
        return std::string( buf, buf + size );
    }

    bool get_bool( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int exists = 0;
        int v = CacheReader_GetBool( handle_, k.data(), k.size(), &exists, 0 );
        if ( exists == 0 )
        {
            throw NotFoundError( "key not found" );
        }
        return v != 0;
    }

    std::int32_t get_int( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int exists = 0;
        int v = CacheReader_GetInteger( handle_, k.data(), k.size(), &exists, 0 );
        if ( exists == 0 )
        {
            throw NotFoundError( "key not found" );
        }
        return ( std::int32_t )v;
    }

    std::int64_t get_long( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int exists = 0;
        long long v = CacheReader_GetLong( handle_, k.data(), k.size(), &exists, ( long long )0 );
        if ( exists == 0 )
        {
            throw NotFoundError( "key not found" );
        }
        return ( std::int64_t )v;
    }

    double get_double( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int exists = 0;
        double v = CacheReader_GetDouble( handle_, k.data(), k.size(), &exists, 0.0 );
        if ( exists == 0 )
        {
            throw NotFoundError( "key not found" );
        }
        return v;
    }

    std::vector<std::string> get_vector( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int count = 0;
        int * sizes = nullptr;
        char ** arr = CacheReader_GetVector( handle_, k.data(), k.size(), &count, &sizes );
        auto free_all = [&]()
        {
            if ( arr )
            {
                for ( int i = 0; i < count; ++i )
                {
                    if ( arr[i] )
                    {
                        free( arr[i] );
                    }
                }
                free( arr );
            }
            if ( sizes )
            {
                free( sizes );
            }
        };
        if ( !arr )
        {
            free_all();
            throw NotFoundError( "key not found" );
        }
        std::vector<std::string> out;
        out.reserve( count );
        for ( int i = 0; i < count; ++i )
        {
            out.emplace_back( arr[i], arr[i] + sizes[i] );
        }
        free_all();
        return out;
    }

    std::vector<float> get_vector_float( py::bytes key ) const
    {
        ensure_init();
        std::string k = key;
        int count = 0;
        float * arr = CacheReader_GetFloatVector( handle_, k.data(), k.size(), &count );
        struct Guard
        {
            float * p;
            ~Guard()
            {
                if ( p )
                {
                    free( p );
                }
            }
        } g{ arr };
        if ( !arr )
        {
            throw NotFoundError( "key not found" );
        }
        return std::vector<float>( arr, arr + count );
    }

    void close()
    {
        if ( handle_ )
        {
            CacheReader_DeleteCppObject( handle_ );
            handle_ = nullptr;
        }
        initialized_ = false;
    }

    ~Reader()
    {
        close();
    }

  private:
    void ensure_init() const
    {
        if ( !initialized_ )
        {
            throw UninitializedError( "CacheReader not initialized; call update()" );
        }
    }
    CacheReaderHandle * handle_{ nullptr };
    bool initialized_{ false };
};

class Writer
{
  public:
    Writer( const std::string & task_name,
            const std::string & settings_location,
            std::uint64_t number_of_key_slots ) :
        handle_( NewCacheWriterHandle() ),
        taskName_( task_name )
    {
        // FIXME: destinationFolder_

        if ( !handle_ )
            throw std::runtime_error( "NewCacheWriterHandle returned null" );
        int ret = CacheWriter_Initialize( handle_, task_name.c_str(),
                                          settings_location.c_str(),
                                          ( uint64_t )number_of_key_slots );
        if ( ret != 0 )
        {
            CacheWriter_DeleteCppObject( handle_ );
            handle_ = nullptr;
            throw std::runtime_error( "CacheWriter_Initialize failed with code " + std::to_string( ret ) );
        }
    }

    void insert_key( py::bytes key, py::bytes val, std::int8_t key_type )
    {
        if ( handle_ == nullptr )
        {
            throw std::runtime_error( "Invalid handle" );
        }

        std::string k = key, v = val;
        int8_t ret = v.empty()
                         ? CacheWriter_InsertKey( handle_, k.data(), k.size(), nullptr, 0, key_type )
                         : CacheWriter_InsertKey( handle_, k.data(), k.size(), v.data(), v.size(), key_type );
        if ( ret != 0 )
        {
            throw std::runtime_error( "InsertKey failed with code " + std::to_string( ( int )ret ) );
        }
    }

    void add_duplicate_value( const std::string & value, std::int8_t query_type )
    {
        if ( handle_ == nullptr )
        {
            throw std::runtime_error( "Invalid handle" );
        }

        CacheWriter_AddDuplicateValue( handle_, value.c_str(), query_type );
    }

    void finish_add_duplicate_values()
    {
        if ( CacheWriter_FinishAddDuplicateValues( handle_ ) != 0 )
        {
            throw std::runtime_error( "FinishAddDuplicateValues failed" );
        }
    }

    std::string finish_cache_creation()
    {
        if ( handle_ == nullptr )
        {
            throw std::runtime_error( "Invalid handle" );
        }

        int ret = CacheWriter_FinishCacheCreation( handle_ );
        if ( ret != 0 )
            throw std::runtime_error( "FinishCacheCreation failed with code " + std::to_string( ret ) );
        // Return an epoch-ms timestamp like your Go code (for convenience)
        long long ms = ( long long )time( nullptr ) * 1000LL;
        auto timestamp = std::to_string( ms );

        // Rename cache file
        auto cacheWithoutTimestamp = destinationFolder_;
        cacheWithoutTimestamp += "/";
        cacheWithoutTimestamp += taskName_;
        cacheWithoutTimestamp += ".cache";

        auto cacheWithTimestamp = destinationFolder_;
        cacheWithTimestamp += "/";
        cacheWithTimestamp += taskName_;
        cacheWithTimestamp += ".";
        cacheWithTimestamp += timestamp;
        cacheWithTimestamp += ".cache";

        // FIXME: error handling
        ::rename( cacheWithoutTimestamp.c_str(), cacheWithTimestamp.c_str() );

        return timestamp;
    }

    void close()
    {
        if ( handle_ )
        {
            CacheWriter_DeleteCppObject( handle_ );
            handle_ = nullptr;
        }
    }
    ~Writer()
    {
        close();
    }

  private:
    CacheWriterHandle * handle_{ nullptr };

    std::string destinationFolder_{ "." };
    std::string taskName_;
};

PYBIND11_MODULE( axoncache, m )
{
    auto NotFound = py::register_exception<NotFoundError>( m, "NotFoundError" );
    auto Uninit = py::register_exception<UninitializedError>( m, "UninitializedError" );

    auto alcacheLogger = []( const char * msg, const axoncache::LogLevel & level )
    {
        switch ( level )
        {
            case axoncache::LogLevel::INFO:
                std::cout << msg << std::endl;
                break;
            case axoncache::LogLevel::WARNING:
                std::cerr << msg << std::endl;
                break;
            case axoncache::LogLevel::ERROR:
                std::cerr << msg << std::endl;
                break;
        }
    };

    // FIXME: this should be optional or configured differently
    if ( getenv( "AXONCACHE_ENABLE_LOGGING" ) )
    {
        axoncache::Logger::setLogFunction( alcacheLogger );
    }

    py::class_<Reader>( m, "Reader" )
        .def( py::init<>() )
        .def( "update", &Reader::update, py::arg( "task_name" ), py::arg( "destination_folder" ), py::arg( "timestamp" ) )
        .def( "contains_key", &Reader::contains_key, py::arg( "key" ) )
        .def( "get_key", &Reader::get_key, py::arg( "key" ) )
        .def( "get_key_type", &Reader::get_key_type, py::arg( "key" ) )
        .def( "get_bool", &Reader::get_bool, py::arg( "key" ) )
        .def( "get_int", &Reader::get_int, py::arg( "key" ) )
        .def( "get_long", &Reader::get_long, py::arg( "key" ) )
        .def( "get_double", &Reader::get_double, py::arg( "key" ) )
        .def( "get_vector", &Reader::get_vector, py::arg( "key" ) )
        .def( "get_vector_float", &Reader::get_vector_float, py::arg( "key" ) )
        .def( "close", &Reader::close );

    py::class_<Writer>( m, "Writer" )
        .def( py::init<const std::string &, const std::string &, std::uint64_t>(),
              py::arg( "task_name" ), py::arg( "settings_location" ), py::arg( "number_of_key_slots" ) )
        .def( "insert_key", &Writer::insert_key, py::arg( "key" ), py::arg( "value" ), py::arg( "key_type" ) )
        .def( "add_duplicate_value", &Writer::add_duplicate_value, py::arg( "value" ), py::arg( "query_type" ) )
        .def( "finish_add_duplicate_values", &Writer::finish_add_duplicate_values )
        .def( "finish_cache_creation", &Writer::finish_cache_creation )
        .def( "close", &Writer::close );

    m.doc() = "Python bindings for AxonCache C API";
}
