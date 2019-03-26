#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <ctype.h>
#include <stdarg.h>

#define INDEX_BIT_COUNT      12
#define LENGTH_BIT_COUNT     4
#define WINDOW_SIZE          ( 1 << INDEX_BIT_COUNT )
#define RAW_LOOK_AHEAD_SIZE  ( 1 << LENGTH_BIT_COUNT )
#define BREAK_EVEN           ( ( 1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT ) / 9 )
#define LOOK_AHEAD_SIZE      ( RAW_LOOK_AHEAD_SIZE + BREAK_EVEN )
#define TREE_ROOT            WINDOW_SIZE
#define END_OF_STREAM        0
#define UNUSED               0
#define MOD_WINDOW( a )      ( ( a ) & ( WINDOW_SIZE - 1 ) )
#define PACIFIER_COUNT 2047
#define SEEK_END 2



typedef struct bit_file {
 FILE *file;
 unsigned char mask;
 int rack;
 int pacifier_counter;
} BIT_FILE;

unsigned char window[ WINDOW_SIZE ];

struct {
    int parent;
    int smaller_child;
    int larger_child;
} tree[ WINDOW_SIZE + 1 ];



BIT_FILE *OpenInputBitFile();
BIT_FILE *OpenOutputBitFile();
void OutputBit();
void OutputBits();
int InputBit();
unsigned long InputBits();
void CloseInputBitFile();
void CloseOutputBitFile();
void FilePrintBinary();
void CompressFile();
void ExpandFile();
void usage_exit();
void print_ratios();
long file_size();
void InitTree();
void ContractNode();
void ReplaceNode();
int FindNextNode();
void DeleteString();
int AddString();
void fatal_error( char *fmt, ... );


extern char *Usage;
extern char *CompressionName;
char *CompressionName = "LZSS";
char *Usage           = "in-file out-file\n\n";

void InitTree( r )
int r;
{
    tree[ TREE_ROOT ].larger_child = r;
    tree[ r ].parent = TREE_ROOT;
    tree[ r ].larger_child = UNUSED;
    tree[ r ].smaller_child = UNUSED;
}


BIT_FILE *OpenOutputBitFile( char *name )
//char *name;
{
BIT_FILE *bit_file;

bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
if ( bit_file == NULL )
   return( bit_file );
bit_file->file = fopen( name, "wb" );
bit_file->rack = 0;
bit_file->mask = 0x80;
bit_file->pacifier_counter = 0;
return( bit_file );
}

BIT_FILE *OpenInputBitFile(char *name )
//char *name;
{
BIT_FILE *bit_file;

bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
if ( bit_file == NULL )
return( bit_file );
bit_file->file = fopen( name, "rb" );
bit_file->rack = 0;
bit_file->mask = 0x80;
bit_file->pacifier_counter = 0;
return( bit_file );
}

void CloseOutputBitFile(BIT_FILE *bit_file )
{
if ( bit_file->mask != 0x80 )
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
fatal_error( "Fatal error in CloseBitFile!\n" );
fclose( bit_file->file );
free( (char *) bit_file );
}

void CloseInputBitFile( BIT_FILE *bit_file )
{
fclose( bit_file->file );
free( (char*) bit_file );
}

void OutputBit(BIT_FILE *bit_file, int bit )

{
if ( bit )
bit_file->rack |= bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 ) {
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
printf("Fatal error in OutputBit!\n" );
if( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
putc( '.', stdout );
bit_file->rack = 0;
bit_file->mask = 0x80;
}
}

void OutputBits(BIT_FILE *bit_file,unsigned long code,int count )
{
unsigned long mask;

mask = 1L << ( count - 1 );
while ( mask != 0) 
{
if ( mask & code )
bit_file->rack |= bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 ) {
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
fatal_error( "Fatal error in OutputBit!\n" );
if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT  ) == 0 )
putc( '.', stdout );
bit_file->rack = 0;
bit_file->mask = 0x80;
}
mask >>= 1;
}
}

int InputBit(BIT_FILE *bit_file )
{
int value;
if ( bit_file->mask == 0x80 ) {
bit_file->rack = getc( bit_file->file );
if ( bit_file->rack == EOF )
fatal_error( "Fatal error in InputBit!\n" );
if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT  ) == 0 )
putc( '.', stdout );
}
value = bit_file->rack & bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 )
bit_file->mask = 0x80;
return ( value ? 1 : 0 );
}

unsigned long InputBits(BIT_FILE *bit_file, int bit_count )
{
unsigned long mask;
unsigned long return_value;

mask = 1L << ( bit_count - 1 );
return_value = 0;
while ( mask != 0) {
 
if ( bit_file->mask == 0x80 ) {
    bit_file->rack = getc( bit_file->file );
    if ( bit_file->rack == EOF )
     fatal_error( "Fatal error in InputBit!\n" );
if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT) == 0 )
     putc( '.', stdout );
}
if ( bit_file->rack & bit_file->mask )
return_value |=mask;
mask >>= 1;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 )
bit_file->mask = 0x80;
}
return( return_value );
}

long file_size(char *name)
{
long eof_ftell;
FILE *file;
file = fopen( name, "r");
if ( file == NULL )
return( 0L );
fseek( file, 0L, SEEK_END );
eof_ftell = ftell( file );
fclose( file );
return( eof_ftell );
}

void print_ratios( char *input, char *output )
{
long input_size;
long output_size;
int ratio;
input_size = file_size( input );
if ( input_size == 0 )
input_size = 1;
output_size =file_size( output );
ratio = 100 - (int) ( output_size * 100L / input_size );
printf( "\nInput bytes:%ld\n", input_size );
printf( "Output bytes:%ld\n", output_size );
if ( output_size == 0 )
output_size = 1;
printf( "Compression ratio: %d%%\n", ratio );
}

void ContractNode( old_node, new_node )
int old_node;
int new_node;
{
    tree[ new_node ].parent = tree[ old_node ].parent;
    if ( tree[ tree[ old_node ].parent ].larger_child == old_node )
        tree[ tree[ old_node ].parent ].larger_child = new_node;
    else
        tree[ tree[ old_node ].parent ].smaller_child = new_node;
    tree[ old_node ].parent = UNUSED;
}

void ReplaceNode( old_node, new_node )
int old_node;
int new_node;
{
    int parent;

    parent = tree[ old_node ].parent;
    if ( tree[ parent ].smaller_child == old_node )
        tree[ parent ].smaller_child = new_node;
    else
        tree[ parent ].larger_child = new_node;
    tree[ new_node ] = tree[ old_node ];
    tree[ tree[ new_node ].smaller_child ].parent = new_node;
    tree[ tree[ new_node ].larger_child ].parent = new_node;
    tree[ old_node ].parent = UNUSED;
}

int FindNextNode( node )
int node;
{
    int next;

    next = tree[ node ].smaller_child;
    while ( tree[ next ].larger_child != UNUSED )
        next = tree[ next ].larger_child;
    return( next );
}

void DeleteString( p )
int p;
{
    int  replacement;

    if ( tree[ p ].parent == UNUSED )
        return;
    if ( tree[ p ].larger_child == UNUSED )
        ContractNode( p, tree[ p ].smaller_child );
    else if ( tree[ p ].smaller_child == UNUSED )
        ContractNode( p, tree[ p ].larger_child );
    else {
        replacement = FindNextNode( p );
        DeleteString( replacement );
        ReplaceNode( p, replacement );
    }
}

int AddString( new_node, match_position )
int new_node;
int *match_position;
{
    int i;
    int test_node;
    int delta;
    int match_length;
    int *child;

    if ( new_node == END_OF_STREAM )
        return( 0 );
    test_node = tree[ TREE_ROOT ].larger_child;
    match_length = 0;
    for ( ; ; ) {
        for ( i = 0 ; i < LOOK_AHEAD_SIZE ; i++ ) {
            delta = window[ MOD_WINDOW( new_node + i ) ] -
                    window[ MOD_WINDOW( test_node + i ) ];
            if ( delta != 0 )
                break;
        }
        if ( i >= match_length ) {
            match_length = i;
            *match_position = test_node;
            if ( match_length >= LOOK_AHEAD_SIZE ) {
                ReplaceNode( test_node, new_node );
                return( match_length );
            }
        }
        if ( delta >= 0 )
            child = &tree[ test_node ].larger_child;
        else
            child = &tree[ test_node ].smaller_child;
        if ( *child == UNUSED ) {
            *child = new_node;
            tree[ new_node ].parent = test_node;
            tree[ new_node ].larger_child = UNUSED;
            tree[ new_node ].smaller_child = UNUSED;
            return( match_length );
        }
        test_node = *child;
    }
}

void CompressFile( input, output, argc, argv )
FILE *input;
BIT_FILE *output;
int argc;
char *argv[];
{
    int i;
    int c;
    int look_ahead_bytes;
    int current_position;
    int replace_count;
    int match_length;
    int match_position;

    current_position = 1;
    for ( i = 0 ; i < LOOK_AHEAD_SIZE ; i++ ) {
        if ( ( c = getc( input ) ) == EOF )
            break;
        window[ current_position + i ] = (unsigned char) c;
    }
    look_ahead_bytes = i;
    InitTree( current_position );
    match_length = 0;
    match_position = 0;
    while ( look_ahead_bytes > 0 ) {
        if ( match_length > look_ahead_bytes )
            match_length = look_ahead_bytes;
        if ( match_length <= BREAK_EVEN ) {
            replace_count = 1;
            OutputBit( output, 1 );
            OutputBits( output,(unsigned long) window[ current_position ], 8 );
        } 
        else {
            OutputBit( output, 0 );
            OutputBits( output,
                        (unsigned long) match_position, INDEX_BIT_COUNT );
            OutputBits( output,
                        (unsigned long) ( match_length - ( BREAK_EVEN + 1 ) ),
                        LENGTH_BIT_COUNT );
            replace_count = match_length;
        }
        for ( i = 0 ; i < replace_count ; i++ ) {
            DeleteString( MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ) );
            if ( ( c = getc( input ) ) == EOF )
                look_ahead_bytes--;
            else
                window[ MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ) ]
                        = (unsigned char) c;
            current_position = MOD_WINDOW( current_position + 1 );
            if ( look_ahead_bytes )
                match_length = AddString( current_position, &match_position );
        }
    };
    OutputBit( output, 0 );
    OutputBits( output, (unsigned long) END_OF_STREAM, INDEX_BIT_COUNT );
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
}

int main(int argc, char *argv[] )
{
    BIT_FILE *output;
    FILE *input;
    setbuf( stdout, NULL );
    input = fopen(argv [ 1 ], "rb" );
    if ( input == NULL )
    fatal_error("Error opening for input\n");
    output = OpenOutputBitFile( argv[ 2 ] );
    if ( output == NULL )
    fatal_error( "Error opening for input\n");
    printf( "\nCompressing %s to %s \n", argv[ 1 ], argv[ 2 ] );
    printf( "Using %s\n", CompressionName );
    CompressFile( input, output, argc-3, argv+3 );
    CloseOutputBitFile( output );
    fclose( input );
    print_ratios( argv[ 1 ], argv[ 2 ] );
    return( 0 );
}

void fatal_error( char *fmt, ...)
{
    va_list argptr;

    va_start( argptr, fmt );
    printf( "\nFatal error: " );
    vprintf( fmt, argptr );
    va_end( argptr );
    exit( -1 );
}