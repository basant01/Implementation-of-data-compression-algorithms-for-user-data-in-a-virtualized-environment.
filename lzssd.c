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
//BIT_FILE *bit_file;
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

void ExpandFile( input, output, argc, argv )
BIT_FILE *input;
FILE *output;
int argc;
char *argv[];
{
    int i;
    int current_position;
    int c;
    int match_length;
    int match_position;

    current_position = 1;
    for ( ; ; ) {
        if ( InputBit( input ) ) {
            c = (int) InputBits( input, 8 );
            putc( c, output );
            window[ current_position ] = (unsigned char) c;
            current_position = MOD_WINDOW( current_position + 1 );
        } else {
            match_position = (int) InputBits( input, INDEX_BIT_COUNT );
            if ( match_position == END_OF_STREAM )
                break;
            match_length = (int) InputBits( input, LENGTH_BIT_COUNT );
            match_length += BREAK_EVEN;
            for ( i = 0 ; i <= match_length ; i++ ) {
                c = window[ MOD_WINDOW( match_position + i ) ];
                putc( c, output );
                window[ current_position ] = (unsigned char) c;
                current_position = MOD_WINDOW( current_position + 1 );
            }
        }
    }
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
printf("\nThe file has been expanded");
}

int main(int argc, char *argv[] )
{
     FILE *output;
    BIT_FILE *input;

    setbuf( stdout, NULL );
    input = OpenInputBitFile( argv[ 1 ] );
    if ( input == NULL )
	fatal_error( "Error opening %s for input\n", argv[ 1 ] );
    output = fopen( argv[ 2 ], "wb" );
    if ( output == NULL )
	fatal_error( "Error opening %s for output\n", argv[ 2 ] );
    printf( "\nExpanding %s to %s\n", argv[ 1 ], argv[ 2 ] );
    printf( "Using %s\n", CompressionName );
    argc -= 3;
    argv += 3;
    ExpandFile( input, output, argc, argv );
    CloseInputBitFile( input );
    fclose( output );
    putc( '\n', stdout );
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