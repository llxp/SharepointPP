/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED

#if defined(ANDROID_NDK) || defined(__BORLANDC__) || defined(__QNXNTO__)
#   include <ctype.h>
#   include <limits.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#	if defined(__PS3__)
#		include <stddef.h>
#	endif
#else
#   include <cctype>
#   include <climits>
#   include <cstdio>
#   include <cstdlib>
#   include <cstring>
#endif
#include <stdint.h>

/*
   TODO: intern strings instead of allocation.
*/
/*
	gcc:
        g++ -Wall -DTINYXML2_DEBUG tinyxml2.cpp xmltest.cpp -o gccxmltest.exe

    Formatting, Artistic Style:
        AStyle.exe --style=1tbs --indent-switches --break-closing-brackets --indent-preprocessor tinyxml2.cpp tinyxml2.h
*/

#if defined( _DEBUG ) || defined (__DEBUG__)
#   ifndef TINYXML2_DEBUG
#       define TINYXML2_DEBUG
#   endif
#endif

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#ifdef _WIN32
#   ifdef TINYXML2_EXPORT
#       define TINYXML2_LIB __declspec(dllexport)
#   elif defined(TINYXML2_IMPORT)
#       define TINYXML2_LIB __declspec(dllimport)
#   else
#       define TINYXML2_LIB
#   endif
#elif __GNUC__ >= 4
#   define TINYXML2_LIB __attribute__((visibility("default")))
#else
#   define TINYXML2_LIB
#endif


#if defined(TINYXML2_DEBUG)
#   if defined(_MSC_VER)
#       // "(void)0," is for suppressing C4127 warning in "assert(false)", "assert(true)" and the like
#       define TIXMLASSERT( x )           if ( !((void)0,(x))) { __debugbreak(); }
#   elif defined (ANDROID_NDK)
#       include <android/log.h>
#       define TIXMLASSERT( x )           if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); }
#   else
#       include <assert.h>
#       define TIXMLASSERT                assert
#   endif
#else
#   define TIXMLASSERT( x )               {}
#endif


/* Versioning, past 1.0.14:
	http://semver.org/
*/
static const int TIXML2_MAJOR_VERSION = 7;
static const int TIXML2_MINOR_VERSION = 0;
static const int TIXML2_PATCH_VERSION = 1;

#define TINYXML2_MAJOR_VERSION 7
#define TINYXML2_MINOR_VERSION 0
#define TINYXML2_PATCH_VERSION 1

// A fixed element depth limit is problematic. There needs to be a
// limit to avoid a stack overflow. However, that limit varies per
// system, and the capacity of the stack. On the other hand, it's a trivial
// attack that can result from ill, malicious, or even correctly formed XML,
// so there needs to be a limit in place.
static const int TINYXML2_MAX_ELEMENT_DEPTH = 100;

namespace tinyxml2
{
class XMLDocument;
class XMLElement;
class XMLAttribute;
class XMLComment;
class XMLText;
class XMLDeclaration;
class XMLUnknown;
class XMLPrinter;

/*
	A class that wraps strings. Normally stores the start and end
	pointers into the XML file itself, and will apply normalization
	and entity translation if actually read. Can also store (and memory
	manage) a traditional char[]

    Isn't clear why TINYXML2_LIB is needed; but seems to fix #719
*/
class TINYXML2_LIB StrPair
{
public:
    enum {
        NEEDS_ENTITY_PROCESSING			= 0x01,
        NEEDS_NEWLINE_NORMALIZATION		= 0x02,
        NEEDS_WHITESPACE_COLLAPSING     = 0x04,

        TEXT_ELEMENT		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_NAME		            = 0,
        ATTRIBUTE_VALUE		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_VALUE_LEAVE_ENTITIES  = NEEDS_NEWLINE_NORMALIZATION,
        COMMENT							= NEEDS_NEWLINE_NORMALIZATION
    };

	__declspec(dllexport) StrPair() : _flags( 0 ), _start( 0 ), _end( 0 ) {}
	__declspec(dllexport) ~StrPair();

	__declspec(dllexport) void Set( char* start, char* end, int flags ) {
        TIXMLASSERT( start );
        TIXMLASSERT( end );
        Reset();
        _start  = start;
        _end    = end;
        _flags  = flags | NEEDS_FLUSH;
    }

	__declspec(dllexport) const char* GetStr();

	__declspec(dllexport) bool Empty() const {
        return _start == _end;
    }

	__declspec(dllexport) void SetInternedStr( const char* str ) {
        Reset();
        _start = const_cast<char*>(str);
    }

	__declspec(dllexport) void SetStr( const char* str, int flags=0 );

	__declspec(dllexport)  char* ParseText( char* in, const char* endTag, int strFlags, int* curLineNumPtr );
	__declspec(dllexport) char* ParseName( char* in );

	__declspec(dllexport) void TransferTo( StrPair* other );
	__declspec(dllexport) void Reset();

private:
	__declspec(dllexport) void CollapseWhitespace();

    enum {
        NEEDS_FLUSH = 0x100,
        NEEDS_DELETE = 0x200
    };

    int     _flags;
    char*   _start;
    char*   _end;

	__declspec(dllexport) StrPair( const StrPair& other );	// not supported
	__declspec(dllexport) void operator=( const StrPair& other );	// not supported, use TransferTo()
};


/*
	A dynamic array of Plain Old Data. Doesn't support constructors, etc.
	Has a small initial memory pool, so that low or no usage will not
	cause a call to new/delete
*/
template <class T, int INITIAL_SIZE>
class DynArray
{
public:
	__declspec(dllexport) DynArray() :
        _mem( _pool ),
        _allocated( INITIAL_SIZE ),
        _size( 0 )
    {
    }

	__declspec(dllexport) ~DynArray() {
        if ( _mem != _pool ) {
            delete [] _mem;
        }
    }

	__declspec(dllexport) void Clear() {
        _size = 0;
    }

	__declspec(dllexport) void Push( T t ) {
        TIXMLASSERT( _size < INT_MAX );
        EnsureCapacity( _size+1 );
        _mem[_size] = t;
        ++_size;
    }

	__declspec(dllexport) T* PushArr( int count ) {
        TIXMLASSERT( count >= 0 );
        TIXMLASSERT( _size <= INT_MAX - count );
        EnsureCapacity( _size+count );
        T* ret = &_mem[_size];
        _size += count;
        return ret;
    }

	__declspec(dllexport) T Pop() {
        TIXMLASSERT( _size > 0 );
        --_size;
        return _mem[_size];
    }

	__declspec(dllexport) void PopArr( int count ) {
        TIXMLASSERT( _size >= count );
        _size -= count;
    }

	__declspec(dllexport) bool Empty() const {
        return _size == 0;
    }

	__declspec(dllexport) T& operator[](int i) {
        TIXMLASSERT( i>= 0 && i < _size );
        return _mem[i];
    }

	__declspec(dllexport) const T& operator[](int i) const {
        TIXMLASSERT( i>= 0 && i < _size );
        return _mem[i];
    }

	__declspec(dllexport) const T& PeekTop() const {
        TIXMLASSERT( _size > 0 );
        return _mem[ _size - 1];
    }

	__declspec(dllexport) int Size() const {
        TIXMLASSERT( _size >= 0 );
        return _size;
    }

	__declspec(dllexport) int Capacity() const {
        TIXMLASSERT( _allocated >= INITIAL_SIZE );
        return _allocated;
    }

	__declspec(dllexport) void SwapRemove(int i) {
		TIXMLASSERT(i >= 0 && i < _size);
		TIXMLASSERT(_size > 0);
		_mem[i] = _mem[_size - 1];
		--_size;
	}

	__declspec(dllexport) const T* Mem() const {
        TIXMLASSERT( _mem );
        return _mem;
    }

	__declspec(dllexport) T* Mem() {
        TIXMLASSERT( _mem );
        return _mem;
    }

private:
	__declspec(dllexport) DynArray( const DynArray& ); // not supported
	__declspec(dllexport) void operator=( const DynArray& ); // not supported

	__declspec(dllexport) void EnsureCapacity( int cap ) {
        TIXMLASSERT( cap > 0 );
        if ( cap > _allocated ) {
            TIXMLASSERT( cap <= INT_MAX / 2 );
            int newAllocated = cap * 2;
            T* newMem = new T[newAllocated];
            TIXMLASSERT( newAllocated >= _size );
            memcpy( newMem, _mem, sizeof(T)*_size );	// warning: not using constructors, only works for PODs
            if ( _mem != _pool ) {
                delete [] _mem;
            }
            _mem = newMem;
            _allocated = newAllocated;
        }
    }

    T*  _mem;
    T   _pool[INITIAL_SIZE];
    int _allocated;		// objects allocated
    int _size;			// number objects in use
};


/*
	Parent virtual class of a pool for fast allocation
	and deallocation of objects.
*/
class MemPool
{
public:
    MemPool() {}
	__declspec(dllexport) virtual ~MemPool() {}

	__declspec(dllexport) virtual int ItemSize() const = 0;
	__declspec(dllexport) virtual void* Alloc() = 0;
	__declspec(dllexport) virtual void Free( void* ) = 0;
	__declspec(dllexport) virtual void SetTracked() = 0;
};


/*
	Template child class to create pools of the correct type.
*/
template< int ITEM_SIZE >
class MemPoolT : public MemPool
{
public:
	__declspec(dllexport) MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), _nAllocs(0), _maxAllocs(0), _nUntracked(0)	{}
	__declspec(dllexport) ~MemPoolT() {
        MemPoolT< ITEM_SIZE >::Clear();
    }

	__declspec(dllexport) void Clear() {
        // Delete the blocks.
        while( !_blockPtrs.Empty()) {
            Block* lastBlock = _blockPtrs.Pop();
            delete lastBlock;
        }
        _root = 0;
        _currentAllocs = 0;
        _nAllocs = 0;
        _maxAllocs = 0;
        _nUntracked = 0;
    }

	__declspec(dllexport) virtual int ItemSize() const	{
        return ITEM_SIZE;
    }
	__declspec(dllexport) int CurrentAllocs() const		{
        return _currentAllocs;
    }

	__declspec(dllexport) virtual void* Alloc() {
        if ( !_root ) {
            // Need a new block.
            Block* block = new Block();
            _blockPtrs.Push( block );

            Item* blockItems = block->items;
            for( int i = 0; i < ITEMS_PER_BLOCK - 1; ++i ) {
                blockItems[i].next = &(blockItems[i + 1]);
            }
            blockItems[ITEMS_PER_BLOCK - 1].next = 0;
            _root = blockItems;
        }
        Item* const result = _root;
        TIXMLASSERT( result != 0 );
        _root = _root->next;

        ++_currentAllocs;
        if ( _currentAllocs > _maxAllocs ) {
            _maxAllocs = _currentAllocs;
        }
        ++_nAllocs;
        ++_nUntracked;
        return result;
    }

	__declspec(dllexport) virtual void Free( void* mem ) {
        if ( !mem ) {
            return;
        }
        --_currentAllocs;
        Item* item = static_cast<Item*>( mem );
#ifdef TINYXML2_DEBUG
        memset( item, 0xfe, sizeof( *item ) );
#endif
        item->next = _root;
        _root = item;
    }
	__declspec(dllexport) void Trace( const char* name ) {
        printf( "Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n",
                name, _maxAllocs, _maxAllocs * ITEM_SIZE / 1024, _currentAllocs,
                ITEM_SIZE, _nAllocs, _blockPtrs.Size() );
    }

	__declspec(dllexport) void SetTracked() {
        --_nUntracked;
    }

	__declspec(dllexport) int Untracked() const {
        return _nUntracked;
    }

	// This number is perf sensitive. 4k seems like a good tradeoff on my machine.
	// The test file is large, 170k.
	// Release:		VS2010 gcc(no opt)
	//		1k:		4000
	//		2k:		4000
	//		4k:		3900	21000
	//		16k:	5200
	//		32k:	4300
	//		64k:	4000	21000
    // Declared public because some compilers do not accept to use ITEMS_PER_BLOCK
    // in private part if ITEMS_PER_BLOCK is private
    enum { ITEMS_PER_BLOCK = (4 * 1024) / ITEM_SIZE };

private:
	__declspec(dllexport) MemPoolT( const MemPoolT& ); // not supported
	__declspec(dllexport) void operator=( const MemPoolT& ); // not supported

    union Item {
        Item*   next;
        char    itemData[ITEM_SIZE];
    };
    struct Block {
        Item items[ITEMS_PER_BLOCK];
    };
    DynArray< Block*, 10 > _blockPtrs;
    Item* _root;

    int _currentAllocs;
    int _nAllocs;
    int _maxAllocs;
    int _nUntracked;
};



/**
	Implements the interface to the "Visitor pattern" (see the Accept() method.)
	If you call the Accept() method, it requires being passed a XMLVisitor
	class to handle callbacks. For nodes that contain other nodes (Document, Element)
	you will get called with a VisitEnter/VisitExit pair. Nodes that are always leafs
	are simply called with Visit().

	If you return 'true' from a Visit method, recursive parsing will continue. If you return
	false, <b>no children of this node or its siblings</b> will be visited.

	All flavors of Visit methods have a default implementation that returns 'true' (continue
	visiting). You need to only override methods that are interesting to you.

	Generally Accept() is called on the XMLDocument, although all nodes support visiting.

	You should never change the document from a callback.

	@sa XMLNode::Accept()
*/
class TINYXML2_LIB XMLVisitor
{
public:
	__declspec(dllexport) virtual ~XMLVisitor() {}

    /// Visit a document.
	__declspec(dllexport) virtual bool VisitEnter( const XMLDocument& /*doc*/ )			{
        return true;
    }
    /// Visit a document.
	__declspec(dllexport) virtual bool VisitExit( const XMLDocument& /*doc*/ )			{
        return true;
    }

    /// Visit an element.
	__declspec(dllexport) virtual bool VisitEnter( const XMLElement& /*element*/, const XMLAttribute* /*firstAttribute*/ )	{
        return true;
    }
    /// Visit an element.
	__declspec(dllexport) virtual bool VisitExit( const XMLElement& /*element*/ )			{
        return true;
    }

    /// Visit a declaration.
	__declspec(dllexport) virtual bool Visit( const XMLDeclaration& /*declaration*/ )		{
        return true;
    }
    /// Visit a text node.
	__declspec(dllexport) virtual bool Visit( const XMLText& /*text*/ )					{
        return true;
    }
    /// Visit a comment node.
	__declspec(dllexport) virtual bool Visit( const XMLComment& /*comment*/ )				{
        return true;
    }
    /// Visit an unknown node.
	__declspec(dllexport) virtual bool Visit( const XMLUnknown& /*unknown*/ )				{
        return true;
    }
};

// WARNING: must match XMLDocument::_errorNames[]
enum XMLError {
    XML_SUCCESS = 0,
    XML_NO_ATTRIBUTE,
    XML_WRONG_ATTRIBUTE_TYPE,
    XML_ERROR_FILE_NOT_FOUND,
    XML_ERROR_FILE_COULD_NOT_BE_OPENED,
    XML_ERROR_FILE_READ_ERROR,
    XML_ERROR_PARSING_ELEMENT,
    XML_ERROR_PARSING_ATTRIBUTE,
    XML_ERROR_PARSING_TEXT,
    XML_ERROR_PARSING_CDATA,
    XML_ERROR_PARSING_COMMENT,
    XML_ERROR_PARSING_DECLARATION,
    XML_ERROR_PARSING_UNKNOWN,
    XML_ERROR_EMPTY_DOCUMENT,
    XML_ERROR_MISMATCHED_ELEMENT,
    XML_ERROR_PARSING,
    XML_CAN_NOT_CONVERT_TEXT,
    XML_NO_TEXT_NODE,
	XML_ELEMENT_DEPTH_EXCEEDED,

	XML_ERROR_COUNT
};


/*
	Utility functionality.
*/
class TINYXML2_LIB XMLUtil
{
public:
	__declspec(dllexport) static const char* SkipWhiteSpace( const char* p, int* curLineNumPtr )	{
        TIXMLASSERT( p );

        while( IsWhiteSpace(*p) ) {
            if (curLineNumPtr && *p == '\n') {
                ++(*curLineNumPtr);
            }
            ++p;
        }
        TIXMLASSERT( p );
        return p;
    }
	__declspec(dllexport) static char* SkipWhiteSpace( char* p, int* curLineNumPtr )				{
        return const_cast<char*>( SkipWhiteSpace( const_cast<const char*>(p), curLineNumPtr ) );
    }

    // Anything in the high order range of UTF-8 is assumed to not be whitespace. This isn't
    // correct, but simple, and usually works.
	__declspec(dllexport) static bool IsWhiteSpace( char p )					{
        return !IsUTF8Continuation(p) && isspace( static_cast<unsigned char>(p) );
    }

	__declspec(dllexport) inline static bool IsNameStartChar( unsigned char ch ) {
        if ( ch >= 128 ) {
            // This is a heuristic guess in attempt to not implement Unicode-aware isalpha()
            return true;
        }
        if ( isalpha( ch ) ) {
            return true;
        }
        return ch == ':' || ch == '_';
    }

	__declspec(dllexport) inline static bool IsNameChar( unsigned char ch ) {
        return IsNameStartChar( ch )
               || isdigit( ch )
               || ch == '.'
               || ch == '-';
    }

	__declspec(dllexport) inline static bool StringEqual( const char* p, const char* q, int nChar=INT_MAX )  {
        if ( p == q ) {
            return true;
        }
        TIXMLASSERT( p );
        TIXMLASSERT( q );
        TIXMLASSERT( nChar >= 0 );
        return strncmp( p, q, nChar ) == 0;
    }

	__declspec(dllexport) inline static bool IsUTF8Continuation( char p ) {
        return ( p & 0x80 ) != 0;
    }

	__declspec(dllexport) static const char* ReadBOM( const char* p, bool* hasBOM );
    // p is the starting location,
    // the UTF-8 value of the entity will be placed in value, and length filled in.
	__declspec(dllexport) static const char* GetCharacterRef( const char* p, char* value, int* length );
	__declspec(dllexport) static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

    // converts primitive types to strings
	__declspec(dllexport) static void ToStr( int v, char* buffer, int bufferSize );
	__declspec(dllexport) static void ToStr( unsigned v, char* buffer, int bufferSize );
	__declspec(dllexport) static void ToStr( bool v, char* buffer, int bufferSize );
	__declspec(dllexport) static void ToStr( float v, char* buffer, int bufferSize );
	__declspec(dllexport) static void ToStr( double v, char* buffer, int bufferSize );
	__declspec(dllexport) static void ToStr(int64_t v, char* buffer, int bufferSize);

    // converts strings to primitive types
	__declspec(dllexport) static bool	ToInt( const char* str, int* value );
	__declspec(dllexport) static bool ToUnsigned( const char* str, unsigned* value );
	__declspec(dllexport) static bool	ToBool( const char* str, bool* value );
	__declspec(dllexport) static bool	ToFloat( const char* str, float* value );
	__declspec(dllexport) static bool ToDouble( const char* str, double* value );
	__declspec(dllexport) static bool ToInt64(const char* str, int64_t* value);

	// Changes what is serialized for a boolean value.
	// Default to "true" and "false". Shouldn't be changed
	// unless you have a special testing or compatibility need.
	// Be careful: static, global, & not thread safe.
	// Be sure to set static const memory as parameters.
	__declspec(dllexport) static void SetBoolSerialization(const char* writeTrue, const char* writeFalse);

private:
	static const char* writeBoolTrue;
	static const char* writeBoolFalse;
};


/** XMLNode is a base class for every object that is in the
	XML Document Object Model (DOM), except XMLAttributes.
	Nodes have siblings, a parent, and children which can
	be navigated. A node is always in a XMLDocument.
	The type of a XMLNode can be queried, and it can
	be cast to its more defined type.

	A XMLDocument allocates memory for all its Nodes.
	When the XMLDocument gets deleted, all its Nodes
	will also be deleted.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	@endverbatim
*/
class TINYXML2_LIB XMLNode
{
    friend class XMLDocument;
    friend class XMLElement;
public:

    /// Get the XMLDocument that owns this XMLNode.
	__declspec(dllexport) const XMLDocument* GetDocument() const {
        TIXMLASSERT( _document );
        return _document;
    }
    /// Get the XMLDocument that owns this XMLNode.
	__declspec(dllexport) XMLDocument* GetDocument() {
        TIXMLASSERT( _document );
        return _document;
    }

    /// Safely cast to an Element, or null.
	__declspec(dllexport) virtual XMLElement* ToElement() {
        return 0;
    }
    /// Safely cast to Text, or null.
	__declspec(dllexport) virtual XMLText* ToText() {
        return 0;
    }
    /// Safely cast to a Comment, or null.
	__declspec(dllexport) virtual XMLComment* ToComment() {
        return 0;
    }
    /// Safely cast to a Document, or null.
	__declspec(dllexport) virtual XMLDocument* ToDocument()	{
        return 0;
    }
    /// Safely cast to a Declaration, or null.
	__declspec(dllexport) virtual XMLDeclaration* ToDeclaration() {
        return 0;
    }
    /// Safely cast to an Unknown, or null.
	__declspec(dllexport) virtual XMLUnknown* ToUnknown() {
        return 0;
    }

	__declspec(dllexport) virtual const XMLElement* ToElement() const {
        return 0;
    }
	__declspec(dllexport) virtual const XMLText*			ToText() const			{
        return 0;
    }
	__declspec(dllexport) virtual const XMLComment*		ToComment() const		{
        return 0;
    }
	__declspec(dllexport) virtual const XMLDocument*		ToDocument() const		{
        return 0;
    }
	__declspec(dllexport) virtual const XMLDeclaration*	ToDeclaration() const	{
        return 0;
    }
	__declspec(dllexport) virtual const XMLUnknown*		ToUnknown() const		{
        return 0;
    }

    /** The meaning of 'value' changes for the specific type.
    	@verbatim
    	Document:	empty (NULL is returned, not an empty string)
    	Element:	name of the element
    	Comment:	the comment text
    	Unknown:	the tag contents
    	Text:		the text string
    	@endverbatim
    */
	__declspec(dllexport) const char* Value() const;

    /** Set the Value of an XML node.
    	@sa Value()
    */
	__declspec(dllexport) void SetValue( const char* val, bool staticMem=false );

    /// Gets the line number the node is in, if the document was parsed from a file.
	__declspec(dllexport) int GetLineNum() const { return _parseLineNum; }

    /// Get the parent of this node on the DOM.
	__declspec(dllexport) const XMLNode*	Parent() const			{
        return _parent;
    }

	__declspec(dllexport) XMLNode* Parent()						{
        return _parent;
    }

    /// Returns true if this node has no children.
	__declspec(dllexport) bool NoChildren() const					{
        return !_firstChild;
    }

    /// Get the first child node, or null if none exists.
	__declspec(dllexport) const XMLNode*  FirstChild() const		{
        return _firstChild;
    }

	__declspec(dllexport) XMLNode*		FirstChild()			{
        return _firstChild;
    }

    /** Get the first child element, or optionally the first child
        element with the specified name.
    */
	__declspec(dllexport) const XMLElement* FirstChildElement( const char* name = 0 ) const;

	__declspec(dllexport) XMLElement* FirstChildElement( const char* name = 0 )	{
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->FirstChildElement( name ));
    }

    /// Get the last child node, or null if none exists.
	__declspec(dllexport) const XMLNode* LastChild() const {
        return _lastChild;
    }

	__declspec(dllexport) XMLNode* LastChild() {
        return _lastChild;
    }

    /** Get the last child element or optionally the last child
        element with the specified name.
    */
	__declspec(dllexport) const XMLElement* LastChildElement( const char* name = 0 ) const;

	__declspec(dllexport) XMLElement* LastChildElement( const char* name = 0 )	{
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->LastChildElement(name) );
    }

    /// Get the previous (left) sibling node of this node.
	__declspec(dllexport) const XMLNode* PreviousSibling() const {
        return _prev;
    }

	__declspec(dllexport) XMLNode* PreviousSibling() {
        return _prev;
    }

    /// Get the previous (left) sibling element of this node, with an optionally supplied name.
	__declspec(dllexport) const XMLElement*	PreviousSiblingElement( const char* name = 0 ) const ;

	__declspec(dllexport) XMLElement* PreviousSiblingElement( const char* name = 0 ) {
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement( name ) );
    }

    /// Get the next (right) sibling node of this node.
	__declspec(dllexport) const XMLNode* NextSibling() const {
        return _next;
    }

	__declspec(dllexport) XMLNode* NextSibling() {
        return _next;
    }

    /// Get the next (right) sibling element of this node, with an optionally supplied name.
	__declspec(dllexport) const XMLElement* NextSiblingElement( const char* name = 0 ) const;

	__declspec(dllexport) XMLElement* NextSiblingElement( const char* name = 0 ) {
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->NextSiblingElement( name ) );
    }

    /**
    	Add a child node as the last (right) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
	__declspec(dllexport) XMLNode* InsertEndChild( XMLNode* addThis );

	__declspec(dllexport) XMLNode* LinkEndChild( XMLNode* addThis )	{
        return InsertEndChild( addThis );
    }
    /**
    	Add a child node as the first (left) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
	__declspec(dllexport) XMLNode* InsertFirstChild( XMLNode* addThis );
    /**
    	Add a node after the specified child node.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the afterThis node
		is not a child of this node, or if the node does not
		belong to the same document.
    */
	__declspec(dllexport) XMLNode* InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );

    /**
    	Delete all the children of this node.
    */
	__declspec(dllexport) void DeleteChildren();

    /**
    	Delete a child of this node.
    */
	__declspec(dllexport) void DeleteChild( XMLNode* node );

    /**
    	Make a copy of this node, but not its children.
    	You may pass in a Document pointer that will be
    	the owner of the new Node. If the 'document' is
    	null, then the node returned will be allocated
    	from the current Document. (this->GetDocument())

    	Note: if called on a XMLDocument, this will return null.
    */
	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const = 0;

	/**
		Make a copy of this node and all its children.

		If the 'target' is null, then the nodes will
		be allocated in the current document. If 'target'
        is specified, the memory will be allocated is the
        specified XMLDocument.

		NOTE: This is probably not the correct tool to
		copy a document, since XMLDocuments can have multiple
		top level XMLNodes. You probably want to use
        XMLDocument::DeepCopy()
	*/
	__declspec(dllexport) XMLNode* DeepClone( XMLDocument* target ) const;

    /**
    	Test if 2 nodes are the same, but don't test children.
    	The 2 nodes do not need to be in the same Document.

    	Note: if called on a XMLDocument, this will return false.
    */
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const = 0;

    /** Accept a hierarchical visit of the nodes in the TinyXML-2 DOM. Every node in the
    	XML tree will be conditionally visited and the host will be called back
    	via the XMLVisitor interface.

    	This is essentially a SAX interface for TinyXML-2. (Note however it doesn't re-parse
    	the XML for the callbacks, so the performance of TinyXML-2 is unchanged by using this
    	interface versus any other.)

    	The interface has been based on ideas from:

    	- http://www.saxproject.org/
    	- http://c2.com/cgi/wiki?HierarchicalVisitorPattern

    	Which are both good references for "visiting".

    	An example of using Accept():
    	@verbatim
    	XMLPrinter printer;
    	tinyxmlDoc.Accept( &printer );
    	const char* xmlcstr = printer.CStr();
    	@endverbatim
    */
	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const = 0;

	/**
		Set user data into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	__declspec(dllexport) void SetUserData(void* userData) { _userData = userData; }

	/**
		Get user data set into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	__declspec(dllexport) void* GetUserData() const { return _userData; }

protected:
	__declspec(dllexport) explicit XMLNode( XMLDocument* );
	__declspec(dllexport) virtual ~XMLNode();

	__declspec(dllexport) virtual char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

    XMLDocument*	_document;
    XMLNode*		_parent;
    mutable StrPair	_value;
    int             _parseLineNum;

    XMLNode*		_firstChild;
    XMLNode*		_lastChild;

    XMLNode*		_prev;
    XMLNode*		_next;

	void*			_userData;

private:
    MemPool*		_memPool;
	__declspec(dllexport) void Unlink( XMLNode* child );
	__declspec(dllexport) static void DeleteNode( XMLNode* node );
	__declspec(dllexport) void InsertChildPreamble( XMLNode* insertThis ) const;
	__declspec(dllexport) const XMLElement* ToElementWithName( const char* name ) const;

	__declspec(dllexport) XMLNode( const XMLNode& );	// not supported
	__declspec(dllexport) XMLNode& operator=( const XMLNode& );	// not supported
};


/** XML text.

	Note that a text node can have child element nodes, for example:
	@verbatim
	<root>This is <b>bold</b></root>
	@endverbatim

	A text node can have 2 ways to output the next. "normal" output
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with
	SetCData() and query it with CData().
*/
class TINYXML2_LIB XMLText : public XMLNode
{
    friend class XMLDocument;
public:
	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

	__declspec(dllexport) virtual XMLText* ToText()			{
        return this;
    }
	__declspec(dllexport) virtual const XMLText* ToText() const	{
        return this;
    }

    /// Declare whether this should be CDATA or standard text.
	__declspec(dllexport) void SetCData( bool isCData )			{
        _isCData = isCData;
    }
    /// Returns true if this is a CDATA text element.
	__declspec(dllexport) bool CData() const						{
        return _isCData;
    }

	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const;
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
	__declspec(dllexport) explicit XMLText( XMLDocument* doc )	: XMLNode( doc ), _isCData( false )	{}
	__declspec(dllexport) virtual ~XMLText()												{}

	__declspec(dllexport) char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
    bool _isCData;

	__declspec(dllexport) XMLText( const XMLText& );	// not supported
	__declspec(dllexport) XMLText& operator=( const XMLText& );	// not supported
};


/** An XML Comment. */
class TINYXML2_LIB XMLComment : public XMLNode
{
    friend class XMLDocument;
public:
	__declspec(dllexport) virtual XMLComment*	ToComment()					{
        return this;
    }
	__declspec(dllexport) virtual const XMLComment* ToComment() const		{
        return this;
    }

	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const;
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
	__declspec(dllexport) explicit XMLComment( XMLDocument* doc );
	__declspec(dllexport) virtual ~XMLComment();

	__declspec(dllexport) char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

private:
	__declspec(dllexport) XMLComment( const XMLComment& );	// not supported
	__declspec(dllexport) XMLComment& operator=( const XMLComment& );	// not supported
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXML-2 will happily read or write files without a declaration,
	however.

	The text of the declaration isn't interpreted. It is parsed
	and written as a string.
*/

class TINYXML2_LIB XMLDeclaration : public XMLNode
{
    friend class XMLDocument;
public:
	__declspec(dllexport) virtual XMLDeclaration*	ToDeclaration()					{
        return this;
    }
	__declspec(dllexport) virtual const XMLDeclaration* ToDeclaration() const		{
        return this;
    }

	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const;
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
	__declspec(dllexport) explicit XMLDeclaration( XMLDocument* doc );
	__declspec(dllexport) virtual ~XMLDeclaration();

	__declspec(dllexport) char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
	__declspec(dllexport) XMLDeclaration( const XMLDeclaration& );	// not supported
	__declspec(dllexport) XMLDeclaration& operator=( const XMLDeclaration& );	// not supported
};


/** Any tag that TinyXML-2 doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into XMLUnknowns.
*/
class TINYXML2_LIB XMLUnknown : public XMLNode
{
    friend class XMLDocument;
public:
	__declspec(dllexport) virtual XMLUnknown*	ToUnknown()					{
        return this;
    }
	__declspec(dllexport) virtual const XMLUnknown* ToUnknown() const		{
        return this;
    }

	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const;
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
	__declspec(dllexport) explicit XMLUnknown( XMLDocument* doc );
	__declspec(dllexport) virtual ~XMLUnknown();

	__declspec(dllexport) char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
	__declspec(dllexport) XMLUnknown( const XMLUnknown& );	// not supported
	__declspec(dllexport) XMLUnknown& operator=( const XMLUnknown& );	// not supported
};



/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not XMLNodes. You may only query the
	Next() attribute in a list.
*/
class TINYXML2_LIB XMLAttribute
{
    friend class XMLElement;
public:
    /// The name of the attribute.
	__declspec(dllexport) const char* Name() const;

    /// The value of the attribute.
	__declspec(dllexport) const char* Value() const;

    /// Gets the line number the attribute is in, if the document was parsed from a file.
	__declspec(dllexport) int GetLineNum() const { return _parseLineNum; }

    /// The next attribute in the list.
	__declspec(dllexport) const XMLAttribute* Next() const {
        return _next;
    }

    /** IntValue interprets the attribute as an integer, and returns the value.
        If the value isn't an integer, 0 will be returned. There is no error checking;
    	use QueryIntValue() if you need error checking.
    */
	__declspec(dllexport) int	IntValue() const {
		int i = 0;
		QueryIntValue(&i);
		return i;
	}

	__declspec(dllexport) int64_t Int64Value() const {
		int64_t i = 0;
		QueryInt64Value(&i);
		return i;
	}

    /// Query as an unsigned integer. See IntValue()
	__declspec(dllexport) unsigned UnsignedValue() const {
        unsigned i=0;
        QueryUnsignedValue( &i );
        return i;
    }
    /// Query as a boolean. See IntValue()
	__declspec(dllexport) bool BoolValue() const {
        bool b=false;
        QueryBoolValue( &b );
        return b;
    }
    /// Query as a double. See IntValue()
	__declspec(dllexport) double DoubleValue() const {
        double d=0;
        QueryDoubleValue( &d );
        return d;
    }
    /// Query as a float. See IntValue()
	__declspec(dllexport) float FloatValue() const {
        float f=0;
        QueryFloatValue( &f );
        return f;
    }

    /** QueryIntValue interprets the attribute as an integer, and returns the value
    	in the provided parameter. The function will return XML_SUCCESS on success,
    	and XML_WRONG_ATTRIBUTE_TYPE if the conversion is not successful.
    */
	__declspec(dllexport) XMLError QueryIntValue( int* value ) const;
    /// See QueryIntValue
	__declspec(dllexport) XMLError QueryUnsignedValue( unsigned int* value ) const;
	/// See QueryIntValue
	__declspec(dllexport) XMLError QueryInt64Value(int64_t* value) const;
	/// See QueryIntValue
	__declspec(dllexport) XMLError QueryBoolValue( bool* value ) const;
    /// See QueryIntValue
	__declspec(dllexport) XMLError QueryDoubleValue( double* value ) const;
    /// See QueryIntValue
	__declspec(dllexport) XMLError QueryFloatValue( float* value ) const;

    /// Set the attribute to a string value.
	__declspec(dllexport) void SetAttribute( const char* value );
    /// Set the attribute to value.
	__declspec(dllexport) void SetAttribute( int value );
    /// Set the attribute to value.
	__declspec(dllexport) void SetAttribute( unsigned value );
	/// Set the attribute to value.
	__declspec(dllexport) void SetAttribute(int64_t value);
	/// Set the attribute to value.
	__declspec(dllexport) void SetAttribute( bool value );
    /// Set the attribute to value.
	__declspec(dllexport) void SetAttribute( double value );
    /// Set the attribute to value.
	__declspec(dllexport) void SetAttribute( float value );

private:
    enum { BUF_SIZE = 200 };

	__declspec(dllexport) XMLAttribute() : _name(), _value(),_parseLineNum( 0 ), _next( 0 ), _memPool( 0 ) {}
	__declspec(dllexport) virtual ~XMLAttribute()	{}

	__declspec(dllexport) XMLAttribute( const XMLAttribute& );	// not supported
	__declspec(dllexport) void operator=( const XMLAttribute& );	// not supported
	__declspec(dllexport) void SetName( const char* name );

	__declspec(dllexport) char* ParseDeep( char* p, bool processEntities, int* curLineNumPtr );

    mutable StrPair _name;
    mutable StrPair _value;
    int             _parseLineNum;
    XMLAttribute*   _next;
    MemPool*        _memPool;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class TINYXML2_LIB XMLElement : public XMLNode
{
    friend class XMLDocument;
public:
    /// Get the name of an element (which is the Value() of the node.)
	__declspec(dllexport) const char* Name() const		{
        return Value();
    }
    /// Set the name of the element.
	__declspec(dllexport) void SetName( const char* str, bool staticMem=false )	{
        SetValue( str, staticMem );
    }

	__declspec(dllexport) virtual XMLElement* ToElement()				{
        return this;
    }
	__declspec(dllexport) virtual const XMLElement* ToElement() const {
        return this;
    }
	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

    /** Given an attribute name, Attribute() returns the value
    	for the attribute of that name, or null if none
    	exists. For example:

    	@verbatim
    	const char* value = ele->Attribute( "foo" );
    	@endverbatim

    	The 'value' parameter is normally null. However, if specified,
    	the attribute will only be returned if the 'name' and 'value'
    	match. This allow you to write code:

    	@verbatim
    	if ( ele->Attribute( "foo", "bar" ) ) callFooIsBar();
    	@endverbatim

    	rather than:
    	@verbatim
    	if ( ele->Attribute( "foo" ) ) {
    		if ( strcmp( ele->Attribute( "foo" ), "bar" ) == 0 ) callFooIsBar();
    	}
    	@endverbatim
    */
	__declspec(dllexport) const char* Attribute( const char* name, const char* value=0 ) const;

    /** Given an attribute name, IntAttribute() returns the value
    	of the attribute interpreted as an integer. The default
        value will be returned if the attribute isn't present,
        or if there is an error. (For a method with error
    	checking, see QueryIntAttribute()).
    */
	__declspec(dllexport) int IntAttribute(const char* name, int defaultValue = 0) const;
    /// See IntAttribute()
	__declspec(dllexport) unsigned UnsignedAttribute(const char* name, unsigned defaultValue = 0) const;
	/// See IntAttribute()
	__declspec(dllexport) int64_t Int64Attribute(const char* name, int64_t defaultValue = 0) const;
	/// See IntAttribute()
	__declspec(dllexport) bool BoolAttribute(const char* name, bool defaultValue = false) const;
    /// See IntAttribute()
	__declspec(dllexport) double DoubleAttribute(const char* name, double defaultValue = 0) const;
    /// See IntAttribute()
	__declspec(dllexport) float FloatAttribute(const char* name, float defaultValue = 0) const;

    /** Given an attribute name, QueryIntAttribute() returns
    	XML_SUCCESS, XML_WRONG_ATTRIBUTE_TYPE if the conversion
    	can't be performed, or XML_NO_ATTRIBUTE if the attribute
    	doesn't exist. If successful, the result of the conversion
    	will be written to 'value'. If not successful, nothing will
    	be written to 'value'. This allows you to provide default
    	value:

    	@verbatim
    	int value = 10;
    	QueryIntAttribute( "foo", &value );		// if "foo" isn't found, value will still be 10
    	@endverbatim
    */
	__declspec(dllexport) XMLError QueryIntAttribute( const char* name, int* value ) const				{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryIntValue( value );
    }

	/// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryUnsignedAttribute( const char* name, unsigned int* value ) const	{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryUnsignedValue( value );
    }

	/// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryInt64Attribute(const char* name, int64_t* value) const {
		const XMLAttribute* a = FindAttribute(name);
		if (!a) {
			return XML_NO_ATTRIBUTE;
		}
		return a->QueryInt64Value(value);
	}

	/// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryBoolAttribute( const char* name, bool* value ) const				{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryBoolValue( value );
    }
    /// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryDoubleAttribute( const char* name, double* value ) const			{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryDoubleValue( value );
    }
    /// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryFloatAttribute( const char* name, float* value ) const			{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryFloatValue( value );
    }

	/// See QueryIntAttribute()
	__declspec(dllexport) XMLError QueryStringAttribute(const char* name, const char** value) const {
		const XMLAttribute* a = FindAttribute(name);
		if (!a) {
			return XML_NO_ATTRIBUTE;
		}
		*value = a->Value();
		return XML_SUCCESS;
	}



    /** Given an attribute name, QueryAttribute() returns
    	XML_SUCCESS, XML_WRONG_ATTRIBUTE_TYPE if the conversion
    	can't be performed, or XML_NO_ATTRIBUTE if the attribute
    	doesn't exist. It is overloaded for the primitive types,
		and is a generally more convenient replacement of
		QueryIntAttribute() and related functions.

		If successful, the result of the conversion
    	will be written to 'value'. If not successful, nothing will
    	be written to 'value'. This allows you to provide default
    	value:

    	@verbatim
    	int value = 10;
    	QueryAttribute( "foo", &value );		// if "foo" isn't found, value will still be 10
    	@endverbatim
    */
	__declspec(dllexport) XMLError QueryAttribute( const char* name, int* value ) const {
		return QueryIntAttribute( name, value );
	}

	__declspec(dllexport) XMLError QueryAttribute( const char* name, unsigned int* value ) const {
		return QueryUnsignedAttribute( name, value );
	}

	__declspec(dllexport) XMLError QueryAttribute(const char* name, int64_t* value) const {
		return QueryInt64Attribute(name, value);
	}

	__declspec(dllexport) XMLError QueryAttribute( const char* name, bool* value ) const {
		return QueryBoolAttribute( name, value );
	}

	__declspec(dllexport) XMLError QueryAttribute( const char* name, double* value ) const {
		return QueryDoubleAttribute( name, value );
	}

	__declspec(dllexport) XMLError QueryAttribute( const char* name, float* value ) const {
		return QueryFloatAttribute( name, value );
	}

	/// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, const char* value )	{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, int value )			{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, unsigned value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }

	/// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute(const char* name, int64_t value) {
		XMLAttribute* a = FindOrCreateAttribute(name);
		a->SetAttribute(value);
	}

	/// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, bool value )			{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, double value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
	__declspec(dllexport) void SetAttribute( const char* name, float value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }

    /**
    	Delete an attribute.
    */
	__declspec(dllexport) void DeleteAttribute( const char* name );

    /// Return the first attribute in the list.
	__declspec(dllexport) const XMLAttribute* FirstAttribute() const {
        return _rootAttribute;
    }
    /// Query a specific attribute in the list.
	__declspec(dllexport) const XMLAttribute* FindAttribute( const char* name ) const;

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, GetText() is limited compared to getting the XMLText child
    	and accessing it directly.

    	If the first child of 'this' is a XMLText, the GetText()
    	returns the character string of the Text node, else null is returned.

    	This is a convenient method for getting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		const char* str = fooElement->GetText();
    	@endverbatim

    	'str' will be a pointer to "This is text".

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then the value of str would be null. The first child node isn't a text node, it is
    	another element. From this XML:
    	@verbatim
    		<foo>This is <b>text</b></foo>
    	@endverbatim
    	GetText() will return "This is ".
    */
	__declspec(dllexport) const char* GetText() const;

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, SetText() is limited compared to creating an XMLText child
    	and mutating it directly.

    	If the first child of 'this' is a XMLText, SetText() sets its value to
		the given string, otherwise it will create a first child that is an XMLText.

    	This is a convenient method for setting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		fooElement->SetText( "Hullaballoo!" );
     	<foo>Hullaballoo!</foo>
		@endverbatim

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then it will not change "This is text", but rather prefix it with a text element:
    	@verbatim
    		<foo>Hullaballoo!<b>This is text</b></foo>
    	@endverbatim

		For this XML:
    	@verbatim
    		<foo />
    	@endverbatim
    	SetText() will generate
    	@verbatim
    		<foo>Hullaballoo!</foo>
    	@endverbatim
    */
	__declspec(dllexport) void SetText( const char* inText );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText( int value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText( unsigned value );
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText(int64_t value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText( bool value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText( double value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
	__declspec(dllexport) void SetText( float value );

    /**
    	Convenience method to query the value of a child text node. This is probably best
    	shown by example. Given you have a document is this form:
    	@verbatim
    		<point>
    			<x>1</x>
    			<y>1.4</y>
    		</point>
    	@endverbatim

    	The QueryIntText() and similar functions provide a safe and easier way to get to the
    	"value" of x and y.

    	@verbatim
    		int x = 0;
    		float y = 0;	// types of x and y are contrived for example
    		const XMLElement* xElement = pointElement->FirstChildElement( "x" );
    		const XMLElement* yElement = pointElement->FirstChildElement( "y" );
    		xElement->QueryIntText( &x );
    		yElement->QueryFloatText( &y );
    	@endverbatim

    	@returns XML_SUCCESS (0) on success, XML_CAN_NOT_CONVERT_TEXT if the text cannot be converted
    			 to the requested type, and XML_NO_TEXT_NODE if there is no child text to query.

    */
	__declspec(dllexport) XMLError QueryIntText( int* ival ) const;
    /// See QueryIntText()
	__declspec(dllexport) XMLError QueryUnsignedText( unsigned* uval ) const;
	/// See QueryIntText()
	__declspec(dllexport) XMLError QueryInt64Text(int64_t* uval) const;
	/// See QueryIntText()
	__declspec(dllexport) XMLError QueryBoolText( bool* bval ) const;
    /// See QueryIntText()
	__declspec(dllexport) XMLError QueryDoubleText( double* dval ) const;
    /// See QueryIntText()
	__declspec(dllexport) XMLError QueryFloatText( float* fval ) const;

	__declspec(dllexport) int IntText(int defaultValue = 0) const;

	/// See QueryIntText()
	__declspec(dllexport) unsigned UnsignedText(unsigned defaultValue = 0) const;
	/// See QueryIntText()
	__declspec(dllexport) int64_t Int64Text(int64_t defaultValue = 0) const;
	/// See QueryIntText()
	__declspec(dllexport) bool BoolText(bool defaultValue = false) const;
	/// See QueryIntText()
	__declspec(dllexport) double DoubleText(double defaultValue = 0) const;
	/// See QueryIntText()
	__declspec(dllexport) float FloatText(float defaultValue = 0) const;

    // internal:
    enum ElementClosingType {
        OPEN,		// <foo>
        CLOSED,		// <foo/>
        CLOSING		// </foo>
    };
	__declspec(dllexport) ElementClosingType ClosingType() const {
        return _closingType;
    }
	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* document ) const;
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
	__declspec(dllexport) char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
	__declspec(dllexport) XMLElement( XMLDocument* doc );
	__declspec(dllexport) virtual ~XMLElement();
	__declspec(dllexport) XMLElement( const XMLElement& );	// not supported
	__declspec(dllexport) void operator=( const XMLElement& );	// not supported

	__declspec(dllexport) XMLAttribute* FindOrCreateAttribute( const char* name );
	__declspec(dllexport) char* ParseAttributes( char* p, int* curLineNumPtr );
	__declspec(dllexport) static void DeleteAttribute( XMLAttribute* attribute );
	__declspec(dllexport) XMLAttribute* CreateAttribute();

    enum { BUF_SIZE = 200 };
    ElementClosingType _closingType;
    // The attribute list is ordered; there is no 'lastAttribute'
    // because the list needs to be scanned for dupes before adding
    // a new attribute.
    XMLAttribute* _rootAttribute;
};


enum Whitespace {
    PRESERVE_WHITESPACE,
    COLLAPSE_WHITESPACE
};


/** A Document binds together all the functionality.
	It can be saved, loaded, and printed to the screen.
	All Nodes are connected and allocated to a Document.
	If the Document is deleted, all its Nodes are also deleted.
*/
class TINYXML2_LIB XMLDocument : public XMLNode
{
    friend class XMLElement;
    // Gives access to SetError and Push/PopDepth, but over-access for everything else.
    // Wishing C++ had "internal" scope.
    friend class XMLNode;
    friend class XMLText;
    friend class XMLComment;
    friend class XMLDeclaration;
    friend class XMLUnknown;
public:
    /// constructor
	__declspec(dllexport) XMLDocument( bool processEntities = true, Whitespace whitespaceMode = PRESERVE_WHITESPACE );
	__declspec(dllexport) ~XMLDocument();

	__declspec(dllexport) virtual XMLDocument* ToDocument()				{
        TIXMLASSERT( this == _document );
        return this;
    }
	__declspec(dllexport) virtual const XMLDocument* ToDocument() const	{
        TIXMLASSERT( this == _document );
        return this;
    }

    /**
    	Parse an XML file from a character string.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.

    	You may optionally pass in the 'nBytes', which is
    	the number of bytes which will be parsed. If not
    	specified, TinyXML-2 will assume 'xml' points to a
    	null terminated string.
    */
	__declspec(dllexport) XMLError Parse( const char* xml, size_t nBytes=(size_t)(-1) );

    /**
    	Load an XML file from disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
	__declspec(dllexport) XMLError LoadFile( const char* filename );

    /**
    	Load an XML file from disk. You are responsible
    	for providing and closing the FILE*.

        NOTE: The file should be opened as binary ("rb")
        not text in order for TinyXML-2 to correctly
        do newline normalization.

    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
	__declspec(dllexport) XMLError LoadFile( FILE* );

    /**
    	Save the XML file to disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
	__declspec(dllexport) XMLError SaveFile( const char* filename, bool compact = false );

    /**
    	Save the XML file to disk. You are responsible
    	for providing and closing the FILE*.

    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
	__declspec(dllexport) XMLError SaveFile( FILE* fp, bool compact = false );

	__declspec(dllexport) bool ProcessEntities() const		{
        return _processEntities;
    }
	__declspec(dllexport) Whitespace WhitespaceMode() const	{
        return _whitespaceMode;
    }

    /**
    	Returns true if this document has a leading Byte Order Mark of UTF8.
    */
	__declspec(dllexport) bool HasBOM() const {
        return _writeBOM;
    }
    /** Sets whether to write the BOM when writing the file.
    */
	__declspec(dllexport) void SetBOM( bool useBOM ) {
        _writeBOM = useBOM;
    }

    /** Return the root element of DOM. Equivalent to FirstChildElement().
        To get the first node, use FirstChild().
    */
	__declspec(dllexport) XMLElement* RootElement()				{
        return FirstChildElement();
    }
	__declspec(dllexport) const XMLElement* RootElement() const	{
        return FirstChildElement();
    }

    /** Print the Document. If the Printer is not provided, it will
        print to stdout. If you provide Printer, this can print to a file:
    	@verbatim
    	XMLPrinter printer( fp );
    	doc.Print( &printer );
    	@endverbatim

    	Or you can use a printer to print to memory:
    	@verbatim
    	XMLPrinter printer;
    	doc.Print( &printer );
    	// printer.CStr() has a const char* to the XML
    	@endverbatim
    */
	__declspec(dllexport) void Print( XMLPrinter* streamer=0 ) const;
	__declspec(dllexport) virtual bool Accept( XMLVisitor* visitor ) const;

    /**
    	Create a new Element associated with
    	this Document. The memory for the Element
    	is managed by the Document.
    */
	__declspec(dllexport) XMLElement* NewElement( const char* name );
    /**
    	Create a new Comment associated with
    	this Document. The memory for the Comment
    	is managed by the Document.
    */
	__declspec(dllexport) XMLComment* NewComment( const char* comment );
    /**
    	Create a new Text associated with
    	this Document. The memory for the Text
    	is managed by the Document.
    */
	__declspec(dllexport) XMLText* NewText( const char* text );
    /**
    	Create a new Declaration associated with
    	this Document. The memory for the object
    	is managed by the Document.

    	If the 'text' param is null, the standard
    	declaration is used.:
    	@verbatim
    		<?xml version="1.0" encoding="UTF-8"?>
    	@endverbatim
    */
	__declspec(dllexport) XMLDeclaration* NewDeclaration( const char* text=0 );
    /**
    	Create a new Unknown associated with
    	this Document. The memory for the object
    	is managed by the Document.
    */
	__declspec(dllexport) XMLUnknown* NewUnknown( const char* text );

    /**
    	Delete a node associated with this document.
    	It will be unlinked from the DOM.
    */
	__declspec(dllexport) void DeleteNode( XMLNode* node );

	__declspec(dllexport) void ClearError() {
        SetError(XML_SUCCESS, 0, 0);
    }

    /// Return true if there was an error parsing the document.
	__declspec(dllexport) bool Error() const {
        return _errorID != XML_SUCCESS;
    }
    /// Return the errorID.
	__declspec(dllexport) XMLError  ErrorID() const {
        return _errorID;
    }
	__declspec(dllexport) const char* ErrorName() const;
	__declspec(dllexport) static const char* ErrorIDToName(XMLError errorID);

    /** Returns a "long form" error description. A hopefully helpful
        diagnostic with location, line number, and/or additional info.
    */
	__declspec(dllexport) const char* ErrorStr() const;

    /// A (trivial) utility function that prints the ErrorStr() to stdout.
	__declspec(dllexport) void PrintError() const;

    /// Return the line where the error occurred, or zero if unknown.
	__declspec(dllexport) int ErrorLineNum() const
    {
        return _errorLineNum;
    }

    /// Clear the document, resetting it to the initial state.
	__declspec(dllexport) void Clear();

	/**
		Copies this document to a target document.
		The target will be completely cleared before the copy.
		If you want to copy a sub-tree, see XMLNode::DeepClone().

		NOTE: that the 'target' must be non-null.
	*/
	__declspec(dllexport) void DeepCopy(XMLDocument* target) const;

	// internal
	__declspec(dllexport) char* Identify( char* p, XMLNode** node );

	// internal
	__declspec(dllexport) void MarkInUse(XMLNode*);

	__declspec(dllexport) virtual XMLNode* ShallowClone( XMLDocument* /*document*/ ) const	{
        return 0;
    }
	__declspec(dllexport) virtual bool ShallowEqual( const XMLNode* /*compare*/ ) const	{
        return false;
    }

private:
	__declspec(dllexport) XMLDocument( const XMLDocument& );	// not supported
	__declspec(dllexport) void operator=( const XMLDocument& );	// not supported

    bool			_writeBOM;
    bool			_processEntities;
    XMLError		_errorID;
    Whitespace		_whitespaceMode;
    mutable StrPair	_errorStr;
    int             _errorLineNum;
    char*			_charBuffer;
    int				_parseCurLineNum;
	int				_parsingDepth;
	// Memory tracking does add some overhead.
	// However, the code assumes that you don't
	// have a bunch of unlinked nodes around.
	// Therefore it takes less memory to track
	// in the document vs. a linked list in the XMLNode,
	// and the performance is the same.
	DynArray<XMLNode*, 10> _unlinked;

    MemPoolT< sizeof(XMLElement) >	 _elementPool;
    MemPoolT< sizeof(XMLAttribute) > _attributePool;
    MemPoolT< sizeof(XMLText) >		 _textPool;
    MemPoolT< sizeof(XMLComment) >	 _commentPool;

	static const char* _errorNames[XML_ERROR_COUNT];

	__declspec(dllexport) void Parse();

	__declspec(dllexport) void SetError( XMLError error, int lineNum, const char* format, ... );

	// Something of an obvious security hole, once it was discovered.
	// Either an ill-formed XML or an excessively deep one can overflow
	// the stack. Track stack depth, and error out if needed.
	class DepthTracker {
	public:
		__declspec(dllexport) explicit DepthTracker(XMLDocument * document) {
			this->_document = document;
			document->PushDepth();
		}
		__declspec(dllexport) ~DepthTracker() {
			_document->PopDepth();
		}
	private:
		XMLDocument * _document;
	};
	__declspec(dllexport) void PushDepth();
	__declspec(dllexport) void PopDepth();

    template<class NodeType, int PoolElementSize>
    NodeType* CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool );
};

template<class NodeType, int PoolElementSize>
inline NodeType* XMLDocument::CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool )
{
    TIXMLASSERT( sizeof( NodeType ) == PoolElementSize );
    TIXMLASSERT( sizeof( NodeType ) == pool.ItemSize() );
    NodeType* returnNode = new (pool.Alloc()) NodeType( this );
    TIXMLASSERT( returnNode );
    returnNode->_memPool = &pool;

	_unlinked.Push(returnNode);
    return returnNode;
}

/**
	A XMLHandle is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that XMLHandle is not part of the TinyXML-2
	DOM structure. It is a separate utility class.

	Take an example:
	@verbatim
	<Document>
		<Element attributeA = "valueA">
			<Child attributeB = "value1" />
			<Child attributeB = "value2" />
		</Element>
	</Document>
	@endverbatim

	Assuming you want the value of "attributeB" in the 2nd "Child" element, it's very
	easy to write a *lot* of code that looks like:

	@verbatim
	XMLElement* root = document.FirstChildElement( "Document" );
	if ( root )
	{
		XMLElement* element = root->FirstChildElement( "Element" );
		if ( element )
		{
			XMLElement* child = element->FirstChildElement( "Child" );
			if ( child )
			{
				XMLElement* child2 = child->NextSiblingElement( "Child" );
				if ( child2 )
				{
					// Finally do something useful.
	@endverbatim

	And that doesn't even cover "else" cases. XMLHandle addresses the verbosity
	of such code. A XMLHandle checks for null pointers so it is perfectly safe
	and correct to use:

	@verbatim
	XMLHandle docHandle( &document );
	XMLElement* child2 = docHandle.FirstChildElement( "Document" ).FirstChildElement( "Element" ).FirstChildElement().NextSiblingElement();
	if ( child2 )
	{
		// do something useful
	@endverbatim

	Which is MUCH more concise and useful.

	It is also safe to copy handles - internally they are nothing more than node pointers.
	@verbatim
	XMLHandle handleCopy = handle;
	@endverbatim

	See also XMLConstHandle, which is the same as XMLHandle, but operates on const objects.
*/
class TINYXML2_LIB XMLHandle
{
public:
    /// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
	__declspec(dllexport) explicit XMLHandle( XMLNode* node ) : _node( node ) {
    }
    /// Create a handle from a node.
	__declspec(dllexport) explicit XMLHandle( XMLNode& node ) : _node( &node ) {
    }
    /// Copy constructor
	__declspec(dllexport) XMLHandle( const XMLHandle& ref ) : _node( ref._node ) {
    }
    /// Assignment
	__declspec(dllexport) XMLHandle& operator=( const XMLHandle& ref )							{
        _node = ref._node;
        return *this;
    }

    /// Get the first child of this handle.
	__declspec(dllexport) XMLHandle FirstChild() 													{
        return XMLHandle( _node ? _node->FirstChild() : 0 );
    }
    /// Get the first child element of this handle.
	__declspec(dllexport) XMLHandle FirstChildElement( const char* name = 0 )						{
        return XMLHandle( _node ? _node->FirstChildElement( name ) : 0 );
    }
    /// Get the last child of this handle.
	__declspec(dllexport) XMLHandle LastChild()													{
        return XMLHandle( _node ? _node->LastChild() : 0 );
    }
    /// Get the last child element of this handle.
	__declspec(dllexport) XMLHandle LastChildElement( const char* name = 0 )						{
        return XMLHandle( _node ? _node->LastChildElement( name ) : 0 );
    }
    /// Get the previous sibling of this handle.
	__declspec(dllexport) XMLHandle PreviousSibling()												{
        return XMLHandle( _node ? _node->PreviousSibling() : 0 );
    }
    /// Get the previous sibling element of this handle.
	__declspec(dllexport) XMLHandle PreviousSiblingElement( const char* name = 0 )				{
        return XMLHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
    }
    /// Get the next sibling of this handle.
	__declspec(dllexport) XMLHandle NextSibling()													{
        return XMLHandle( _node ? _node->NextSibling() : 0 );
    }
    /// Get the next sibling element of this handle.
	__declspec(dllexport) XMLHandle NextSiblingElement( const char* name = 0 )					{
        return XMLHandle( _node ? _node->NextSiblingElement( name ) : 0 );
    }

    /// Safe cast to XMLNode. This can return null.
	__declspec(dllexport) XMLNode* ToNode()							{
        return _node;
    }
    /// Safe cast to XMLElement. This can return null.
	__declspec(dllexport) XMLElement* ToElement() 					{
        return ( _node ? _node->ToElement() : 0 );
    }
    /// Safe cast to XMLText. This can return null.
	__declspec(dllexport) XMLText* ToText() 							{
        return ( _node ? _node->ToText() : 0 );
    }
    /// Safe cast to XMLUnknown. This can return null.
	__declspec(dllexport) XMLUnknown* ToUnknown() 					{
        return ( _node ? _node->ToUnknown() : 0 );
    }
    /// Safe cast to XMLDeclaration. This can return null.
	__declspec(dllexport) XMLDeclaration* ToDeclaration() 			{
        return ( _node ? _node->ToDeclaration() : 0 );
    }

private:
    XMLNode* _node;
};


/**
	A variant of the XMLHandle class for working with const XMLNodes and Documents. It is the
	same in all regards, except for the 'const' qualifiers. See XMLHandle for API.
*/
class TINYXML2_LIB XMLConstHandle
{
public:
	__declspec(dllexport) explicit XMLConstHandle( const XMLNode* node ) : _node( node ) {
    }
	__declspec(dllexport) explicit XMLConstHandle( const XMLNode& node ) : _node( &node ) {
    }
	__declspec(dllexport) XMLConstHandle( const XMLConstHandle& ref ) : _node( ref._node ) {
    }

	__declspec(dllexport) XMLConstHandle& operator=( const XMLConstHandle& ref )							{
        _node = ref._node;
        return *this;
    }

	__declspec(dllexport) const XMLConstHandle FirstChild() const											{
        return XMLConstHandle( _node ? _node->FirstChild() : 0 );
    }
	__declspec(dllexport) const XMLConstHandle FirstChildElement( const char* name = 0 ) const				{
        return XMLConstHandle( _node ? _node->FirstChildElement( name ) : 0 );
    }
	__declspec(dllexport) const XMLConstHandle LastChild()	const										{
        return XMLConstHandle( _node ? _node->LastChild() : 0 );
    }
	__declspec(dllexport) const XMLConstHandle LastChildElement( const char* name = 0 ) const				{
        return XMLConstHandle( _node ? _node->LastChildElement( name ) : 0 );
    }
	__declspec(dllexport) const XMLConstHandle PreviousSibling() const									{
        return XMLConstHandle( _node ? _node->PreviousSibling() : 0 );
    }
	__declspec(dllexport) const XMLConstHandle PreviousSiblingElement( const char* name = 0 ) const		{
        return XMLConstHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
    }
	__declspec(dllexport) const XMLConstHandle NextSibling() const										{
        return XMLConstHandle( _node ? _node->NextSibling() : 0 );
    }
	__declspec(dllexport) const XMLConstHandle NextSiblingElement( const char* name = 0 ) const			{
        return XMLConstHandle( _node ? _node->NextSiblingElement( name ) : 0 );
    }


	__declspec(dllexport) const XMLNode* ToNode() const				{
        return _node;
    }
	__declspec(dllexport) const XMLElement* ToElement() const			{
        return ( _node ? _node->ToElement() : 0 );
    }
	__declspec(dllexport) const XMLText* ToText() const				{
        return ( _node ? _node->ToText() : 0 );
    }
	__declspec(dllexport) const XMLUnknown* ToUnknown() const			{
        return ( _node ? _node->ToUnknown() : 0 );
    }
	__declspec(dllexport) const XMLDeclaration* ToDeclaration() const	{
        return ( _node ? _node->ToDeclaration() : 0 );
    }

private:
    const XMLNode* _node;
};


/**
	Printing functionality. The XMLPrinter gives you more
	options than the XMLDocument::Print() method.

	It can:
	-# Print to memory.
	-# Print to a file you provide.
	-# Print XML without a XMLDocument.

	Print to Memory

	@verbatim
	XMLPrinter printer;
	doc.Print( &printer );
	SomeFunction( printer.CStr() );
	@endverbatim

	Print to a File

	You provide the file pointer.
	@verbatim
	XMLPrinter printer( fp );
	doc.Print( &printer );
	@endverbatim

	Print without a XMLDocument

	When loading, an XML parser is very useful. However, sometimes
	when saving, it just gets in the way. The code is often set up
	for streaming, and constructing the DOM is just overhead.

	The Printer supports the streaming case. The following code
	prints out a trivially simple XML file without ever creating
	an XML document.

	@verbatim
	XMLPrinter printer( fp );
	printer.OpenElement( "foo" );
	printer.PushAttribute( "foo", "bar" );
	printer.CloseElement();
	@endverbatim
*/
class TINYXML2_LIB XMLPrinter : public XMLVisitor
{
public:
    /** Construct the printer. If the FILE* is specified,
    	this will print to the FILE. Else it will print
    	to memory, and the result is available in CStr().
    	If 'compact' is set to true, then output is created
    	with only required whitespace and newlines.
    */
	__declspec(dllexport) XMLPrinter( FILE* file=0, bool compact = false, int depth = 0 );
	__declspec(dllexport) virtual ~XMLPrinter()	{}

    /** If streaming, write the BOM and declaration. */
	__declspec(dllexport) void PushHeader( bool writeBOM, bool writeDeclaration );
    /** If streaming, start writing an element.
        The element must be closed with CloseElement()
    */
	__declspec(dllexport) void OpenElement( const char* name, bool compactMode=false );
    /// If streaming, add an attribute to an open element.
	__declspec(dllexport) void PushAttribute( const char* name, const char* value );
	__declspec(dllexport) void PushAttribute( const char* name, int value );
	__declspec(dllexport) void PushAttribute( const char* name, unsigned value );
	__declspec(dllexport) void PushAttribute(const char* name, int64_t value);
	__declspec(dllexport) void PushAttribute( const char* name, bool value );
	__declspec(dllexport) void PushAttribute( const char* name, double value );
    /// If streaming, close the Element.
	__declspec(dllexport) virtual void CloseElement( bool compactMode=false );

    /// Add a text node.
	__declspec(dllexport) void PushText( const char* text, bool cdata=false );
    /// Add a text node from an integer.
	__declspec(dllexport) void PushText( int value );
    /// Add a text node from an unsigned.
	__declspec(dllexport) void PushText( unsigned value );
	/// Add a text node from an unsigned.
	__declspec(dllexport) void PushText(int64_t value);
	/// Add a text node from a bool.
	__declspec(dllexport) void PushText( bool value );
    /// Add a text node from a float.
	__declspec(dllexport) void PushText( float value );
    /// Add a text node from a double.
	__declspec(dllexport) void PushText( double value );

    /// Add a comment
	__declspec(dllexport) void PushComment( const char* comment );

	__declspec(dllexport) void PushDeclaration( const char* value );
	__declspec(dllexport) void PushUnknown( const char* value );

	__declspec(dllexport) virtual bool VisitEnter( const XMLDocument& /*doc*/ );
	__declspec(dllexport) virtual bool VisitExit( const XMLDocument& /*doc*/ )			{
        return true;
    }

	__declspec(dllexport) virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* attribute );
	__declspec(dllexport) virtual bool VisitExit( const XMLElement& element );

	__declspec(dllexport) virtual bool Visit( const XMLText& text );
	__declspec(dllexport) virtual bool Visit( const XMLComment& comment );
	__declspec(dllexport) virtual bool Visit( const XMLDeclaration& declaration );
	__declspec(dllexport) virtual bool Visit( const XMLUnknown& unknown );

    /**
    	If in print to memory mode, return a pointer to
    	the XML file in memory.
    */
	__declspec(dllexport) const char* CStr() const {
        return _buffer.Mem();
    }
    /**
    	If in print to memory mode, return the size
    	of the XML file in memory. (Note the size returned
    	includes the terminating null.)
    */
	__declspec(dllexport) int CStrSize() const {
        return _buffer.Size();
    }
    /**
    	If in print to memory mode, reset the buffer to the
    	beginning.
    */
	__declspec(dllexport) void ClearBuffer() {
        _buffer.Clear();
        _buffer.Push(0);
		_firstElement = true;
    }

protected:
	__declspec(dllexport) virtual bool CompactMode( const XMLElement& )	{ return _compactMode; }

	/** Prints out the space before an element. You may override to change
	    the space and tabs used. A PrintSpace() override should call Print().
	*/
	__declspec(dllexport) virtual void PrintSpace( int depth );
	__declspec(dllexport) void Print( const char* format, ... );
	__declspec(dllexport) void Write( const char* data, size_t size );
	__declspec(dllexport) inline void Write( const char* data )           { Write( data, strlen( data ) ); }
	__declspec(dllexport) void Putc( char ch );

	__declspec(dllexport) void SealElementIfJustOpened();
    bool _elementJustOpened;
    DynArray< const char*, 10 > _stack;

private:
	__declspec(dllexport) void PrintString( const char*, bool restrictedEntitySet );	// prints out, after detecting entities.

    bool _firstElement;
    FILE* _fp;
    int _depth;
    int _textDepth;
    bool _processEntities;
	bool _compactMode;

    enum {
        ENTITY_RANGE = 64,
        BUF_SIZE = 200
    };
    bool _entityFlag[ENTITY_RANGE];
    bool _restrictedEntityFlag[ENTITY_RANGE];

    DynArray< char, 20 > _buffer;

    // Prohibit cloning, intentionally not implemented
	__declspec(dllexport) XMLPrinter( const XMLPrinter& );
	__declspec(dllexport) XMLPrinter& operator=( const XMLPrinter& );
};


}	// tinyxml2

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // TINYXML2_INCLUDED
