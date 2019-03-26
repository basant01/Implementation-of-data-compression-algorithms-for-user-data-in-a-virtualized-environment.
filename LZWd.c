#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <stdarg.h>

#define BITS                       12
#define MAX_CODE                   ( ( 1 << BITS ) - 1 )
#define TABLE_SIZE                 5021
#define END_OF_STREAM              256
#define FIRST_CODE                 257
#define UNUSED                     -1
#define PACIFIER_COUNT 2047
#define SEEK_END 2


typedef struct bit_file {
 FILE *file;
 unsigned char mask;
 int rack;
 int pacifier_counter;
} BIT_FILE;

struct dictionary {
    int code_value;
    int parent_code;
    char character;
} dict[ TABLE_SIZE ];

char decode_stack[ TABLE_SIZE ];


BIT_FILE *OpenInputBitFile();
BIT_FILE *OpenOutputBitFile();
void OutputBit();
void OutputBits();
int InputBit();
unsigned long InputBits();
void CloseInputBitFile();
void CloseOutputBitFile();
void FilePrintBinary();
void ExpandFile();
void usage_exit();
long file_size();
unsigned int find_child_node();
unsigned int decode_string();
void fatal_error(char *fmt, ... ); 

extern char *Usage;
extern char *CompressionName;
char *CompressionName = "LZW";
char *Usage           = "in-file out-file\n\n";


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
/*BIT_FILE *bit_file;
int bit;*/
{
if ( bit )
bit_file->rack |= bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 ) {
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
printf("Fatal error in OutputBit!\n" );
fatal_error( "Fatal error in OutputBit!\n" );
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

/*void FilePrintBinary( FILE *file,unsigned int code, int bits )
{
unsigned int mask;
mask = 1 << ( bits - 1 );
while ( mask != 0 )
{
if ( code & mask )
fputc( '1', file );
else
fputc( '0', file);
mask >>= 1;
}
}*/

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

void ExpandFile(BIT_FILE *input,FILE *output, int argc, char *argv[] )
{
    unsigned int next_code;
    unsigned int new_code;
    unsigned int old_code;
    int character;
    unsigned int count;

    next_code = FIRST_CODE;
    old_code = (unsigned int) InputBits( input, BITS );
    if ( old_code == END_OF_STREAM )
        return;
    character = old_code;
    putc( old_code, output );

    while ( ( new_code = (unsigned int) InputBits( input, BITS ) )
              != END_OF_STREAM ) {
        if ( new_code >= next_code ) {
            decode_stack[ 0 ] = (char) character;
            count = decode_string( 1, old_code );
        }
        else
            count = decode_string( 0, new_code );
        character = decode_stack[ count - 1 ];
        while ( count > 0 )
            putc( decode_stack[ --count ], output );
        if ( next_code <= MAX_CODE ) {
            dict[ next_code ].parent_code = old_code;
            dict[ next_code ].character = (char) character;
            next_code++;
        }
        old_code = new_code;
    }
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
}

unsigned int decode_string(unsigned int count, unsigned int code )
{
    while ( code > 255 ) {
        decode_stack[ count++ ] = dict[ code ].character;
        code = dict[ code ].parent_code;
    }
    decode_stack[ count++ ] = (char) code;
    return( count );
}

unsigned int find_child_node( int parent_code, int child_character )
{
    int index;
    int offset;

    index = ( child_character << ( BITS - 8 ) ) ^ parent_code;
    if ( index == 0 )
        offset = 1;
    else
        offset = TABLE_SIZE - index;
    for ( ; ; ) {
        if ( dict[ index ].code_value == UNUSED )
            return( index );
        if ( dict[ index ].parent_code == parent_code &&
             dict[ index ].character == (char) child_character )
            return( index );
        index -= offset;
        if ( index < 0 )
            index += TABLE_SIZE;
    }
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
    printf( "Fatal error: " );
    vprintf( fmt, argptr );
    va_end( argptr );
    exit( -1 );
}