/** Sub0Pub core header-only library
 * @remark C++ Type-based Subscriber-Publisher messaging model for embedded, desktop, games, and distributed systems.
 * 
 *  This file is part of Sub0Pub. Original project source available at https://github.com/Crog/Sub0Pub/blob/master/sub0pub.hpp
 * 
 *  MIT License
 *
 * Copyright (c) 2018 Craig Hutchinson <craig-sub0pub@crog.uk>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files 
 *  (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, 
 *  publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do 
 *  so, subject to the following conditions:
 * 
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 *  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef CROG_SUB0PUB_HPP
#define CROG_SUB0PUB_HPP

#include <algorithm>
#include <cassert> //< assert
#include <cstring> //< std::strcmp
#include <array> //< std::array @todo Should we not use this one occurrence for C++98 compatibility?
//#include <typeinfo> //< typeid()
#include <type_traits> //< std::is_same

 /// @todo 0 vs nullptr C++11 only
#if 1 /// @todo cstdint not always available ... C++11/C99 only 
    #include <cstdint> //< uint32_t
#else
    typedef unsigned char uint8_t;
    typedef unsigned int uint32_t;
#endif

/** Logging output for event tracing
 * Define SUB0PUB_TRACE=true to enable message logging to std::cout for event trace, SUB0PUB_TRACE=false
 */
#ifndef SUB0PUB_TRACE
#define SUB0PUB_TRACE false ///< Disable Trace logging to std::cout by default
#endif

/** Assertion based error handling 
 * Define SUB0PUB_ASSERT=true to enable assertion checks for events, SUB0PUB_ASSERT=false to disable
 */
#ifndef SUB0PUB_ASSERT
#define SUB0PUB_ASSERT true ///< Enable assertion tests by default
#endif

#ifndef SUB0PUB_STD
#define SUB0PUB_STD false ///< Use STD ostream and IStream types (May increase binary compiled size)
#endif

#ifndef SUB0PUB_TYPEIDNAME
#define SUB0PUB_TYPEIDNAME false ///< Types given unique/user-defined type index and string name for diagnostics and IPC
#endif

#ifndef SUB0_EXPERIMENTAL
#define SUB0_EXPERIMENTAL false ///< Experimental functionality that may be later removed/dropped
#endif

/** Helper macro for stringifying value using compiler preprocessor
 * e.g. SUB0_STRINGIFY_HELPER(123) == "123", SUB0_STRINGIFY_HELPER(FooBar) == "FooBar"
 * @param  x  A value whos value will be converted to string e.g. FooBar == "FooBar", 123 = "123"
 */
#define SUB0_STRINGIFY_HELPER(x) #x

/** Helper macro for stringifying define using compiler preprocessor
 * e.g. SUB0_STRINGIFY_HELPER(__LINE__) == "123??"
 * @param  x  A macro definition whos value will be converted to string  e.g. __LINE__ == "123??"
 */
#define SUB0_STRINGIFY(x) SUB0_STRINGIFY_HELPER(x)

#if SUB0PUB_STD
#include <ostream> //< std::ostream
#include <istream> //< std::istream
#endif

/// @todo Trace interface - currently std::cout only!!
#if SUB0PUB_TRACE
#include <iostream>
#endif

/** Sub0Pub top-level namespace
*/
namespace sub0
{
    /** Internal utility functions
    */
    namespace utility
    {
        /** Create 4byte packed value at compile time
         * @tparam a,b,c,d  Characters which will be packed into 4-byte uint32_t value
         */
        template <const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d>
        struct FourCC
        {
            static const uint32_t value = (((((d << 8) | c) << 8) | b) << 8) | a;
        };

        /** Hash a string using djb2 hash
         * @param[in] str  Null-terminated string to calculate hash of
         * @todo Implement as compile time with name
         * @return djb2 hash value for input 'str'
         */
        inline uint32_t hash( const char* str)
        {
            uint32_t hash = 5381U;
            for ( ; str[0U] != '\0'; ++str )
            {
                hash = ((hash << 5) + hash) + str[0U]; /* hash * 33 + c */
            }
            return hash;
        }

#if SUB0PUB_STD
        typedef std::ostream OStream;
        typedef std::istream IStream;

        template< typename Type_t >
        inline bool write(OStream& stream, const Type_t& value )
        {
            return stream.write(reinterpret_cast<const char*>(&value), sizeof(value)).good();
        }

        template< typename Type_t >
        inline bool write(OStream& stream)
        {
            const Type_t defaulted;
            return stream.write(reinterpret_cast<const char*>(&defaulted), sizeof(defaulted)).good();
        }
#else
        /**
        * @note char* to unify interface against std::ostream
        */
        class OStream
        {
        public:
            typedef uint_fast32_t StreamSize;

            virtual StreamSize write(const char* const buffer, const StreamSize bufferCount) = 0;

            /** Clear all buffers for this stream and causes any buffered data to be written to the underlying device.
            */
            virtual void flush() = 0;
        };

        /**
        * @note char* to unify interface against std::istream
        */
        class IStream
        {
        public:
            typedef uint_fast32_t StreamSize;

            virtual StreamSize read(char* const buffer, const StreamSize bufferCount) = 0;

            /** Discards specified number of characters from inputSequence
            * @note Setting std::numeric_limits<std::streamsize>::max() discards ONLY the currently buffered bytes
            * @return The number of bytes ignored
            */
            virtual StreamSize ignore( const StreamSize bufferCount ) = 0;

            /** Discards specified number of characters from inputSequence until the specified delimiter is found
            * @note The delimiting character is extracted, and thus the next input operation will continue on the character that follows it (if any).
            * @warning This function may (TBC) block if there isn't any data in the stream
            * @return The number of bytes ignored including the delimiter character
            */
            virtual StreamSize ignore(const StreamSize bufferCount, const char delimiter ) = 0;

            /** Returns whether end of stream has been reached
             * @note For Files this is explicit but for a pipe (e.g. TCP or command pipe '|' ) this may never occur until the pipe is forcefully closed by the other end etc
             * @return True if no more data, false otherwise
            */
            virtual bool isEof() = 0;
        };

        template< typename Type_t >
        inline bool write(OStream& stream, const Type_t& value)
        {
            return stream.write(reinterpret_cast<const char*>(&value), sizeof(value)) == sizeof(value);
        }

        template< typename Type_t >
        inline bool write(OStream& stream)
        {
            const Type_t defaulted;
            return stream.write(reinterpret_cast<const char*>(&defaulted), sizeof(defaulted)) == sizeof(defaulted);
        }
#endif

        template<>
        inline bool write<void>(OStream& stream)
        {
            return true;
        }

    } // END: utility
    
    typedef utility::OStream OStream;
    typedef utility::IStream IStream;

    /** Broker manages publisher-subscriber connection for a 'Data' type
     * @tparam Data  Data type which this instance manages connections for
     */
    template< typename Data >
    class Broker;

    /** Base type for publishing signals of 'Data' type
     * @tparam Data  Data type which this instance manages publishing for
     */
    template< typename Data >
    class Publish;

    /** Base type for subscription to receive signals of 'Data' type
     * @tparam Data  Data type which this instance manages subscriptions for
     */
    template< typename Data >
    class Subscribe;
    
    /** Internal configured details for tracing and error handling
     */
    namespace detail
    {
        /** Provides debug assertion/exception checks for Broker<>
         * @tparam cMessageTrace   Enable logging for broker events
         * @tparam cDoAssert         Enable assertion tests for invalid parameters
         */
        template< const bool cMessageTrace = false, const bool cDoAssert = true >
        struct CheckT
        {
            /** Diagnose creation of new subscriber
             * @param broker  Broker instance that manages the connection
             * @param subscriber  Subscriber to be registered into the broker
             * @param subscriptionCount  Count of existing registered subscriptions on the broker
             * @param subscriptionCapacity  Count specifying subscriptionCount limit for the broker
             */
            template<typename Data>
            inline static void onSubscription( const Broker<Data>& broker, Subscribe<Data>* subscriber, const uint32_t subscriptionCount, const uint32_t subscriptionCapacity )
            {
                if ( cDoAssert )
                {
                    assert( subscriber );
                    assert( subscriptionCount < subscriptionCapacity );
                }
#if SUB0PUB_TRACE /// @todo iostream removal:
                if ( cMessageTrace )
                { 
                    std::cout << "[Sub0Pub] New Subscription " << *subscriber << " for Broker<" << broker.typeName() << ">{" << broker << '}' << std::endl;
                }
#endif
            }

            /** Diagnose creation of new publisher
             * @param publisher  Publisher to be registered into the broker
             * @param broker  Broker instance that manages the connection
             * @param publisherCount  Count of existing registered publishers on the broker
             * @param publisherCapacity  Count specifying publisherCount limit for the broker
             */
            template<typename Data>
            inline static void onPublication( Publish<Data>* publisher, const Broker<Data>& broker, const uint32_t publisherCount, const uint32_t publisherCapacity )
            {
                if ( cDoAssert )
                {
                    assert( publisher );
                    assert( publisherCount < publisherCapacity );
                }
#if SUB0PUB_TRACE /// @todo iostream removal:
                if ( cMessageTrace )
                { 
                    std::cout << "[Sub0Pub] New Publication " << *publisher << " for Broker<" << broker.typeName() << ">{" << broker << '}' << std::endl;
                }
#endif
            }

            /** Diagnose data publish event
             * @param publisher  Publisher that is sending the data
             * @param data  The data to be published
             */
            template<typename Data>
            inline static void onPublish( const Publish<Data>& publisher, const Data& data )
            {
#if SUB0PUB_TRACE /// @todo iostream removal: 
                if ( cMessageTrace )
                {
                    (void)data; ///< @todo Data serialize
                    std::cout << "[Sub0Pub] Published " << publisher
                        << " {_data_todo_}"/** @todo Data serialize: << data*/ << '[' << Broker<Data>::typeName() << ']' << std::endl;
                }
#endif
            }

            /** Diagnose data receive event
             * @param subscriber  Subscriber that is receiving the data
             * @param data  The data that is received
             */
            template<typename Data>
            static void 
#if __GNUG__ /// @todo GCC 7.2(TBC) publish(const Data & data) bug on inlining this function and calling detail::Check::onReceive()?
                __attribute__((noinline)) 
#endif
                onReceive( Subscribe<Data>* subscriber, const Data& data )
            {
                if ( cDoAssert )
                {
#if 0 ///< @temp
                    while (!subscriber)
                    {
                        delay(1);
                    }
#endif
                    if ( subscriber == nullptr )
                    {
                       // while (true) {};
                       assert(subscriber );
                    }
                }
#if SUB0PUB_TRACE /// @todo iostream removal: 
                if ( cMessageTrace )
                {
                    (void)data; ///< @todo Data serialize
                    std::cout << "[Sub0Pub] Received " << *subscriber
                        << " {_data_todo_}"/** @todo Data serialize: << data*/ << '[' << Broker<Data>::typeName() << ']' << std::endl;
                }
#endif
            }
        };

        /** Runtime checker type with support for assert/exception/trace etc
         */
        typedef CheckT<SUB0PUB_TRACE,SUB0PUB_ASSERT> Check;
    } // END: detail

#pragma warning(push)
#pragma warning(disable:4355) ///< warning C4355: 'this' : used in base member initializer list

    /** Base type for an object that subscribes to some strong-typed Data
     * @tparam  Data  Type that will be received from publishers of corresponding type
     */
    template< typename Data >
    class Subscribe
    {
    public:
        /** Registers the subscriber within the broker framework
         * @param[in] typeName Optional unique data name given to data for inter-process signalling. @warning If not supplied non-portable compiler generated names 'may' be used.
         */
        Subscribe( 
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/ 
#endif
        )
        : broker_( this
#if SUB0PUB_TYPEIDNAME
            , typeId, typeName 
#endif
        )
        {}

        virtual ~Subscribe()
        {  broker_.unsubscribe(this); } ///< @todo Make implicit broker handle
        
        /** Receive published Data
         * @remark Data is published from Publish<Data>::publish
         */
        virtual void receive( const Data& data ) = 0;

        virtual bool filter(const Data& data)
        {  return true; }

#if SUB0PUB_TYPEIDNAME
        /** Get name identifier of the Data from the broker
         * @return Broker null-terminated type name
        */
        const char* typeName() const
        { return broker_.typeName(); }

        /** Stream operator for diagnostics reporting
         * @param stream  Stream to report into
         * @param subscriber  Subscriber instance to be written into stream
         * @return Reference to 'stream'
         */
        friend OStream& operator<< ( OStream& stream, const Subscribe<Data>& subscriber )
        { return stream << subscriber.typeName() << '{' << (void*)&subscriber << '}'; }
#endif

    private:
        Broker<Data> broker_; ///< MonoState broker instance to manage publish-subscribe connections
    };

    /** Base type for an object that publishes to some strong-typed Data
     * @tparam  Data  Type that will be published by this object to subscribers of corresponding type
     */
    template< typename Data >
    class Publish
    {
    public:
        /** Registers the publisher within the broker framework
         * @param[in] typeName Optional unique data name given to data for inter-process signaling. @warning If not supplied non-portable compiler generated names 'may' be used.
         */
        Publish(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/
#endif
        )
        : broker_( this
#if SUB0PUB_TYPEIDNAME
            , typeId, typeName
#endif
        )
        {}

        virtual ~Publish()
        { broker_.unsubscribe(this); } ///< @todo Make implicit broker handle

        /** Publish data to subscribers
         * @param[in]  data  Data value to publish to subscribers
         * @remark Data will be received by Subscribe<Data>::receive
         */
        void publish( const Data& data ) const
        {
            detail::Check::onPublish( *this, data );
            broker_.publish(data); //< @todo Add 'this' as traceability to data source for broker specialisation etc
        }

#if SUB0PUB_TYPEIDNAME
        /** Get name identifier of the Data from the broker
         * @return Broker null-terminated type name
        */
        const char* typeName() const
        { return broker_.typeName(); }

        /** Get unique identifier of the Data from the broker
         * @return Broker unique type index
        */
        uint32_t typeId() const
        { return broker_.typeId(); }

        /** Stream operator for diagnostics reporting
         * @param stream  Stream to report into
         * @param publisher  Publisher instance to be written into stream
         * @return Reference to 'stream'
         */
        friend OStream& operator<< ( OStream& stream, const Publish<Data>& publisher )
        { return stream << publisher.typeName() << '{' << (void*)&publisher << '}'; }
#endif

    private:
        Broker<Data> broker_; ///< MonoState broker instance to manage publish-subscribe connections
    };

#pragma warning(pop)

    /** Broker manages publisher-subscriber connection for a data-type
     * @tparam Data  Data type which this instance manages connections for
     * @todo Cross-module support
     */
    template< typename Data >
    class Broker
    {
    public:
        static const uint32_t cMaxSubscriptions = 8U; ///< Subscription limit in fixed table per broker

    public:
        /** Registers subscriber in brokers subscription table
         * @param[in] typeName Optional unique data name given to data for inter-process signaling. 
         * @warning If typeName not supplied compiler generated names 'may' be used which are non-portable between vendors.
         */
        Broker( Subscribe<Data>* subscriber
#if SUB0PUB_TYPEIDNAME
            , const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/ 
#endif
        )
        {
            detail::Check::onSubscription( *this, subscriber, state_.subscriptionCount, cMaxSubscriptions );
#if SUB0PUB_TYPEIDNAME
            setDataName(typeId, typeName);
#endif
            state_.subscriptions[state_.subscriptionCount++] = subscriber;
        }

        /** Validated publication
         * @remark No record of publishers of data is currently maintained
         * @param[in] typeName Optional unique data name given to data for inter-process signalling. 
         * @warning If typeName not supplied compiler generated names 'may' be used which are non-portable between vendors.
         */
        Broker ( Publish<Data>* publisher
#if SUB0PUB_TYPEIDNAME
            , const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/
#endif
        )
        {
            detail::Check::onPublication( publisher, *this, 0, 1/* @note No limit at present */ );
#if SUB0PUB_TYPEIDNAME
            setDataName(typeId, typeName);
#endif
            // Do nothing for now...
        }

        void unsubscribe(Subscribe<Data>* subscriber)
        {
            Subscribe<Data>** const iBegin = state_.subscriptions;
            Subscribe<Data>** const iEnd = iBegin + state_.subscriptionCount;
            Subscribe<Data>** const iPend = std::remove(iBegin, iEnd, subscriber );
            assert(std::distance(iPend,iEnd ) == 1);
            --state_.subscriptionCount;
        }

        void unsubscribe(Publish<Data>* publisher)
        {
            // Do nothing for now...
        }

#if SUB0PUB_TYPEIDNAME
        /** Set a unique identifier for the data the broker manages
         * @remark This name is used during serialisation for inter-process communications
         * @param[in]  typeName  Null terminated compile-time string constant
         */
        void setDataName(const uint32_t typeId, const char* const typeName )
        {
            if (typeId)
            {
                // Check if assigning a different name or Id is when already set
                assert( !state_.typeId || (state_.typeId==typeId) );// @todo use RuntimeCheck and handle if a subscriber uses a different name better
                state_.typeId = typeId; /// @todo sub0::utility::hash(state_.typeName); // Cache hash result @todo Make compile time
            }

            if (typeName)
            {
                // Check if assigning a different name or Id is when already set
                assert( !state_.typeName || (std::strcmp(state_.typeName,typeName)==0) );// @todo use RuntimeCheck and handle if a subscriber uses a different name better
                state_.typeName = typeName;
            }
        }
#endif

        /** Send data to registered subscribers
         * @param data  Data sent to subscribers via their 'receive()' function
         */
        void publish(const Data& data) const
        {
            for (uint32_t iSubscription = 0U; iSubscription < state_.subscriptionCount; ++iSubscription )
            {
                Subscribe<Data>* subscription = state_.subscriptions[iSubscription];
                detail::Check::onReceive( subscription, data );

                if ( subscription->filter(data))
                {
                    subscription->receive(data);
                }
            }
        }

        /** Prints address of monotonic state
         * @param stream  Stream to output into
         * @param broker  Broker instance to output for
         * @return The 'stream' instance
         */
#if 0 ///@todo Remove unecessary stream operations: 
        friend OStream& operator<< ( OStream& stream, const Broker<Data>& broker )
        {
            return stream << (void*)&broker.state_;
        }
#endif

#if SUB0PUB_TYPEIDNAME
        /** @return Unique identifier index for inter-process binary connections
         */
        static uint32_t typeId()
        {
            return state_.typeId;
        }

        /** @return Unique identifier name for inter-process text connections
         */
        static const char* typeName()
        {
            return state_.typeName;
        }
#endif

    private:
        /** Object state as monotonic object shared by all instances
         */
        struct State
        {
            uint32_t subscriptionCount; ///< Count of subscriptions_
            Subscribe<Data>* subscriptions[cMaxSubscriptions];    ///< Subscription table @todo More flexible count-support
#if SUB0PUB_TYPEIDNAME
            uint32_t typeId; ///< Type identifier index or name hash
            const char* typeName; ///< user defined data name overrides non-portable compiler-generated name
#endif
            State() 
                : subscriptionCount(0)
                , subscriptions() 
            {}
        };
        static State state_; ///< MonoState subscription table
    };

    /** Monotonic broker state
     * @todo State should be shared across module boundaries and owned/defined in a single module e.g. std::cout like singleton
     */
    template<typename Data>
    typename Broker<Data>::State Broker<Data>::state_ = Broker<Data>::State();

    /** Explicit allocation of monotonic state
    @note Enables appearing within Globals for ELF embedded targets
    */
#define SUB0_BROKERSTATE(Data) \
    namespace sub0 {  template<> Broker<Data>::State Broker<Data>::state_ = Broker<Data>::State(); } 
    
    /** Publish data, used when inheriting from multiple Publish<> base types
     * @remark Circumvents C++ Name-Hiding limitations when multiple Publish<> base types are present 
        i.e. publish( 1.0F) is ambiguous in this case.
     * @note Compiler error will occur if From does not inherit Publish<Data>
     *
     * @param[in] from  Producer object inheriting from one or more Publish<> objects
     * @param[in] data  Data that will be published using the base Publish<Data> object of From
     */
    template<typename From, typename Data>
    inline void publish(From& from, const Data& data)
    {
        const Publish<Data>& publisher = from;
        publisher.publish(data);
    }

    /** @see publish(const From&,const Data&)
    */
    template<typename From, typename Data>
    inline void publish(From* const from, const Data& data)
    {
        assert(from != nullptr);
        publish(*from, data);
    }

#if SUB0_EXPERIMENTAL //< @todo Decide if this should be part of the API to allow calling from global code easily... better make it the users duty? i.e.e use of static non-owner traceability of data sources could compilcate future features?
    /** C-compatibile global-publish wihout use of registered Publisher
    * @warning Use with caution as each instantiation creates a static
    * @see publish(const From&,const Data&)
    */
    template<typename Data>
    inline void publish_cstatic( const Data& data)
    {
        static Publish<Data> publisher = {}; ///< Create an object instance @note SUB0_EXPERIMENTAL
        publish(publisher, data);
    }
#endif

    /** Interface for data provider to indicate destination buffer status
     * @see ForwardPublish
     */
    class IPublish
    {
    public:

        /** Publish the data owned by the object
         */
        virtual void publish() = 0;
    };

    template< typename Prefix_t
            , typename Header_t
            , typename Postfix_t >
    class BinaryWriter
    {
    public:
        /** Output header and pay-load for data as binary
         * @param stream  Stream to write into
         * @param data  Data to construct a header record and data payload for
         */
        template<typename Data>
        inline bool write(OStream& stream, const Data& data) const
        {
            Header_t header(data);
            return utility::write<Prefix_t>(stream)
                && utility::write(stream, Header_t(data) )
                && utility::write(stream, data )
                && utility::write<Postfix_t>(stream);
        }

        void close( OStream& stream  )
        {
            /* Do nothing */
        }

    };

    struct Buffer
    {
        char* buffer; ///< Data buffer @note a nullptr buffer may be set for unsupported payloads where paddingSize != 0 is required
        uint_fast16_t bufferSize; ///< size of buffer
        uint_fast16_t paddingSize; ///< size of buffer padding data to discard after buffer @note For protocol version compatibility when payloads grow
        IPublish* publisher; ///< Type specific publish of buffer
    };

    /** @tparam  cMaxDataBufferCount  Defines the maximum number of Data type buffers the deserializer can store
    */
    template< typename Header_t, uint_fast16_t cMaxDataBufferCount = 64U >
    class BufferRegister
    {
        typedef std::pair<Header_t,Buffer> HeaderToBuffer;
        typedef std::array<HeaderToBuffer, cMaxDataBufferCount> HeaderToBufferLookup;

    public:
        BufferRegister()
            : registry_()
            , registryEnd_(registry_.begin())
        {}

        /** Register a sink to the specified typed Data buffer
         * @remark Performs insertion sorting on buffers by the IPublish::typeId() for the buffer
         * @todo Make search meahcnism selectable i.e. Array-index, hash, or binary-lookup etc
         * @remark Called by sub0::ForwardPublish<Data>
         *
         * @param[in] publisher  Buffer handling object to store and signal data completion
         * @param[in] paddingSize  Number of trailing bytes after sizeof(Data) has been consumed to ignore/discard 
         *                         for alignment or protocol-version compatibility
         */
        template < typename Data >
        void set(Data& buffer, IPublish& publisher, const uint_fast16_t paddingSize = 0U )
        {
            set( Header_t(buffer)
               , Buffer{
                      reinterpret_cast<char*>(&buffer)
                    , static_cast<uint_fast16_t>(sizeof(buffer))
                    , paddingSize
                    , &publisher 
               } );
        }

        void set(const Header_t& header, const Buffer& buffer)
        {
            /// @todo make this a linked list to remove capacity limitations?
            assert(registryEnd_ < std::end(registry_)); //< Capacity reached

            typename HeaderToBufferLookup::iterator iInsert = std::lower_bound(std::begin(registry_), registryEnd_, header,
                [](const HeaderToBuffer& lhs, const Header_t& rhs) { return lhs.first < rhs; });

            const bool exists = (iInsert != registryEnd_) && (iInsert->first == header);
            if (!exists) //< Insert new entry at location
            {
                std::move_backward(iInsert, registryEnd_, registryEnd_ + 1U);
                ++registryEnd_;
                iInsert->first = header;
            }

            iInsert->second = buffer;
        }

        Buffer find(const Header_t header)
        {
            typename HeaderToBufferLookup::iterator iFind = std::lower_bound(std::begin(registry_), registryEnd_, HeaderToBuffer(header, Buffer())
                , [](const HeaderToBuffer& lhs, const HeaderToBuffer& rhs) { return lhs.first < rhs.first; });

            if ((iFind != registryEnd_) && (iFind->first == header))
                return iFind->second;
            else
                return { nullptr, 0U , 0U, nullptr };
        }

        /** Default validation check against provided header
         * @note No validation occurs by default and processing is pushed onto find() to perform respective lookup operation
         * @todo Unify find/validate so that find returns a handle that can be validated or buffer accessed etc i.e. Iterator or the likes!
         * @param header Header data to validate against
         * @return True always
        */
        bool validate(const Header_t& header) const
        {
            return true;
        }

        void close()///< @TODO This is here as a use-case contained stream state within the buffer map! Remove/deprecate this when/as possible
        {
            /** Do nothing - no state to clear */
        }

    private:
        HeaderToBufferLookup registry_;
        typename HeaderToBufferLookup::iterator registryEnd_; ///< Iterator to end of registry_ @note Count = registryEnd_-registry_
    };

    template< typename Prefix_t, typename Header_t, typename Postfix_t, typename BufferRegister = BufferRegister<Header_t> >
    class BinaryReader
    {
        enum class State { 
              Prefix///< [optional] Prefix-Delimiter is being read
            , Header ///< Data-Header  is being read
            , Data ///< Data payload is being  read
            , Postfix ///< [optional] Postfix-Delimiter is being read

            , SyncLost ///< Error state entered when an error occurs in any state i.e. Corrupted input stream

            , COUNT_ 
        };

    public:
        BinaryReader()
            : dataBufferRegistery_()
            , currentBuffer_()
            , state_()
            , prefix_()
            , header_()
            , postfix_()
        {}

        bool read(IStream& stream)
        {
            /// Read data until an incomplete message
            while (readBuffer(stream))
            {
                if (state_ == State::Header)
                    return true; ///< @return True = Completed reading a payload so allow processing by caller
            }

            return false; ///< @return False = Need more data
        }

        template < typename Data >
        void setDataPublisher(Data& dataBuffer, IPublish& publisher)
        {
            assert(!currentBuffer_.buffer); /// @todo We don't intend to support adding buffers while stream is being processed?
            dataBufferRegistery_.set(dataBuffer, publisher);
        }

        void close( IStream& stream  )
        {
            dataBufferRegistery_.close(); ///< @TODO This is here as a use-case contained stream state wihin the buffer map! Remove/deprecate this when/as possible
            currentBuffer_ = {};
            state_ = {};
        }

    private:

        /** Returns/finds buffer for state
        */
        Buffer findStateBuffer(const State state)
        {
            switch (state)
            {
            default: //< @todo unreachable unless SyncLost
            case State::Prefix: 
                return {reinterpret_cast<char*>(&prefix_), static_cast<uint_fast16_t>( !std::is_void<Prefix_t>::value ? sizeof(prefix_) : 0U), 0U, nullptr};
            case State::Header: 
                return {reinterpret_cast<char*>(&header_), static_cast<uint_fast16_t>(sizeof(header_)), 0U, nullptr };
            case State::Data:   
                return dataBufferRegistery_.find(header_);
            case State::Postfix: 
                return {reinterpret_cast<char*>(&postfix_), static_cast<uint_fast16_t> ( !std::is_void<Postfix_t>::value ? sizeof(postfix_) : 0U), 0U, currentBuffer_.publisher };
            }
        }
        
        /** Read payload data from stream and detect payload completion
         * @return True when data packet(s) have been published, false if no completed packet was present in stream
        */
        bool readBuffer(IStream& stream)
        {
            if (currentBuffer_.bufferSize > 0)
            {
#if SUB0PUB_STD
                const uint_fast16_t readCount = static_cast<uint_fast16_t>(stream.read(currentBuffer_.buffer, currentBuffer_.bufferSize).gcount()); ///< @todo readsome() for async
#else
                const uint_fast16_t readCount = stream.read(currentBuffer_.buffer, currentBuffer_.bufferSize);
#endif
                currentBuffer_.buffer += readCount;
                currentBuffer_.bufferSize -= readCount;

                /// If buffer not complete then we need to return and await more data
                if (currentBuffer_.bufferSize > 0)
                    return false;
            }

            if (currentBuffer_.paddingSize > 0)
            {
#if SUB0PUB_STD
                const uint_fast16_t ignoreCount = static_cast<uint_fast16_t>(stream.ignore(currentBuffer_.paddingSize).gcount()); ///< @todo readsome() for async
#else
                const uint_fast16_t ignoreCount = stream.ignore(currentBuffer_.paddingSize);
#endif
                currentBuffer_.paddingSize -= ignoreCount;

                /// If padding not complete then we need to return and await more data
                /// @todo We could publish the data before completion of the padding... however we cannot check for a post-fix delimiter without doing pad first!?
                if (currentBuffer_.paddingSize > 0)
                    return false;
            }

            //If we got here the buffer and any padding has been read from the stream
            return stateComplete();
        }

        constexpr bool getStateStatus(const State state) const
        {
            switch (state)
            {
            default: //< @todo SyncLost
            case State::Prefix:  return true; ///< @todo Handle non-void Prefix_t: (prefix_ == Prefix_t())
            case State::Header:  return dataBufferRegistery_.validate(header_);
            case State::Data:    return true;
            case State::Postfix: return postfix_ == Postfix_t();///< @todo Handle void Postfix_t
            }
        }

        constexpr State nextState(const State currentState ) const
        {
            switch (currentState)
            {
            default: //< @todo unreachable
            case State::Prefix:  return State::Header;
            case State::Header:  return State::Data;
            case State::Data:    return !std::is_void<Postfix_t>::value ? State::Postfix : nextState(State::Postfix); ///< @note may not have Prefix_t or Postfix_t
            case State::Postfix:
                assert(currentBuffer_.publisher);
                if (currentBuffer_.publisher)
                    currentBuffer_.publisher->publish(); // Signal completion of buffer content to publish data signal
                return !std::is_void<Prefix_t>::value ? State::Prefix : nextState(State::Prefix);
            }
        }

        bool checkStatusOfState(const State currentState) const
        {
            const bool stateStatus = getStateStatus(currentState);
            if(stateStatus)
                return true;

            const char* failureMessage = nullptr;
            switch(currentState)
            {
                case State::Header: failureMessage = "Binary-Header mismatch - stream corruption or incompatible data-stream"; break;
                case State::Postfix: failureMessage = "Binary-Postfix mismatch - stream corruption or incompatible data-stream"; break;
                default: failureMessage = "Sync-Lost - TODO Details"; break;
            }

            if(failureMessage != nullptr)
            {
#if __cpp_exceptions
                throw std::runtime_error(failureMessage);
#else
                assert((void*)0 == failureMessage);
#endif
            }

            return false;
        }

        bool stateComplete()
        {
            if(!checkStatusOfState(state_))
            {
                state_ = State::SyncLost;
                return false;
            }

            state_ = nextState( state_ );
            currentBuffer_ = findStateBuffer(state_);

            // Check if header maps to a recognised Data
            if ( currentBuffer_.buffer == nullptr)/// @todo Does not handle and discard unrecognised typeId [Critical]
            {
                const char* failureMessage = nullptr;
                if ( state_ == State::Data )
                    failureMessage = "Sub0Pub - Data buffer is null, potential payload size mismatch or unrecognised Id"; /// @todo Does not handle changed data structure size [Critical]
                else
                    failureMessage = "Sub0Pub - some logic is wrong!";
               
                if(failureMessage != nullptr)
                {
#if __cpp_exceptions
                    throw std::runtime_error(failureMessage);
#else
                    assert((void*)0 == failureMessage);
#endif
                }
            }
            
            return currentBuffer_.buffer != nullptr;
        }

    private:
        BufferRegister dataBufferRegistery_;
        Buffer currentBuffer_; ///< Current prefix/header/payload/postfix buffer
        State state_; ///< Which buffer is being read

        /// @{ Pre/Post-fix_t can be void which must be mapped to a useable member type
        /// @todo Can we ommit the void members all together instead?
        typedef typename std::conditional<std::is_void<Prefix_t>::value, char, Prefix_t>::type MemberPrefix_t;
        typedef typename std::conditional<std::is_void<Postfix_t>::value,char,Postfix_t>::type MemberPostfix_t;
        /// @}

        MemberPrefix_t prefix_;
        Header_t header_; ///< Packet head buffer
        MemberPostfix_t postfix_;
    };

    /** Binary protocol for serialised signal and data transfer
     * @remark The protocol consists of a Header chunk followed by Header::dataBytes bytes of payload data
     */
    class DefaultSerialisation
    {
        struct Prefix
        {
            const uint32_t magic = sub0::utility::FourCC<'S', 'U', 'B', '0'>::value; //< Magic number to identify Sub0 network protocol packets
        };

        /** Header containing signal type information
        */
        struct Header
        {
            uint32_t typeId; ///< Data type identifier @note The Id may be user specified for inter-process
            uint32_t dataBytes; ///< Count of bytes that follow after the header data

            Header() = default;

            /** header for specified Data type
            */
            template<typename Data>
            Header( const Data& data )
#if SUB0PUB_TYPEIDNAME
                : typeId(Broker<Data>::typeId() )
#else

                : typeId(12345) // reinterpret_cast<uint32_t>(&typeid(data)) ) ///< @todo Crude using typeid address!!!
#endif
                , dataBytes(sizeof(Data) )
            {}

            /** Sort by typeId only
            */
            bool operator < (const Header& rhs) { return typeId < rhs.typeId; }

            /** Compare full equality 
            */
            bool operator == (const Header& rhs) { return (typeId == rhs.typeId) && (dataBytes == rhs.dataBytes); }
        };

        struct Postfix
        {
            const uint8_t delim = '\n';
        };

        using Writer = BinaryWriter<Prefix, Header, Postfix>;
        using Reader = BinaryReader<Prefix, Header, Postfix>;
    };

    /** Serialises Sub0Pub data into a target stream object
     * @remark Serialised data can be received and published using the counterpart StreamDeserializer instance
     * @remark Can be used to create inter-process transfers very easily using the specified Protocol @see sub0::DefaultSerialisation
     * @tparam  Protocol  Stream data protocol to use defining how the data header and payload is structured
     */
    template< typename Protocol = DefaultSerialisation >
    class StreamSerializer
    {
    public:
        /** Construct from stream
         * @param[in] stream  Stream reference stored and used to write serialised data into
         */
        StreamSerializer( OStream& stream )
            : stream_(stream)
            , writer_()
        {}

        /** Receives forwarded data from a subscriber and serialises it to the output stream
         * @param[in] data  Forwarded data
         */
        template<typename Data>
        void forward( const Data& data )
        {
            writer_.write( stream_, data );
        }

        /** Reset writer internal  state
        */
        void close()
        {
            writer_.close( stream_ );
            stream_.flush();
        }

    private:
        OStream& stream_; ///< Stream into which data is serialised
        typename Protocol::Writer writer_;
    };


    /** Publishes messages from a serialised-input stream using the specified Protocol 
     * @remark StreamDeserializer can be used for inter-process or distributed systems over a network where the stream
     *  could be a TcpStream or could be a file in simple cases. The serialised data is expected to be generated from a
     *  corresponding StreamSerializer instance for the same Protocol.
     * @tparam  Protocol  Stream data protocol to use defining how the data header and payload is structured
     */
    template< typename Protocol = DefaultSerialisation >
    class StreamDeserializer
    {
    public:
        /** Store reference to supplied IStream which will be read on update()
        */
        StreamDeserializer( IStream& stream )
            : stream_(stream)
            , reader_()
        {}

        template < typename Data >
        void setDataPublisher( Data& dataBffer, IPublish& publisher )
        {
            reader_.setDataPublisher(dataBffer, publisher );
        }

        /** Polls data from the input stream
         * @return True when data packet(s) have been published, false if no completed packet was present in stream
         */
        bool update()
        {
            return reader_.read(stream_);
        }

        /** Reset reader internal  state
        */
        void close()
        {
            reader_.close( stream_ );
        }

    private:
        IStream& stream_; ///< Stream from which data is de-serialized
        typename Protocol::Reader reader_;
    };

    /** Forward receive() to  Target type convertible from this
     * @remark The call is made with Data type allowing for templated forward() handler functions @see class StreamSerializer
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardSubscribe<..>
     * @tparam  Data  Data type which will be forwarded to the derived Target implementation
     * @tparam  Target  Type of derived class which implements a function of type Target::forward( const Data& data ) via base inheritance or direct member
     */
    template<typename Data, typename Target >
    class ForwardSubscribe : public Subscribe<Data>
    {
    public:
        /** Create subscription to the data type
         * @param typeName  Unique name given to the serialised data entry @note Replaces compiler generated name which is not portable
         */
        ForwardSubscribe(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/
#endif
        )
            : Subscribe<Data>(
#if SUB0PUB_TYPEIDNAME
                typeId, typeName
#endif
                )
        {}

        /** Receives subscribed data and forward to target object
         * @param data  Data to forward
         */
        inline void receive( const Data& data ) final
        {
            static_cast<Target*>(this)->forward( data );
        }
    };

    /** Register publication of data with a provider instance
     * @remark The call is made with Data type allowing for templated forward() handler functions @see class StreamSerializer
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardPublish<..>
     * @tparam  Data  Data type which will be read into from a DataProvider
     * @tparam  DataProvider  CRTP Type of derived class which implements a function of type DataProvider::setDataPublisher( Data&, IPublish& ) via base inheritance or direct member
     *
     * @todo API not final
     */
    template<typename Data, typename DataProvider >
    class ForwardPublish : public Publish<Data>, protected IPublish
    {
    public:
        /** Register publisher buffer with the data provider
         * @param typeName  Unique name given to the serialised data entry @note Replaces compiler generated name which is not portable
         */
        ForwardPublish(
#if SUB0PUB_TYPEIDNAME            
            const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/ 
#endif
        )
            : Publish<Data>(
#if SUB0PUB_TYPEIDNAME
                typeId, typeName
#endif
              )
            , IPublish()
        {
            DataProvider& provider = static_cast<DataProvider&>(*this);
            provider.setDataPublisher( buffer_, static_cast<IPublish&>(*this) ); // Register the buffer sink to the data provider
        }

    private:

        /** Publish the data populated in buffer_
         */
        virtual void publish() final
        { Publish<Data>::publish( buffer_ ); }

    private:
        Data buffer_; ///< Data buffer to be published 
                      ///< @todo Double-buffer data storage for asynchronous processing and receive?
    };

} // END: sub0

#endif
