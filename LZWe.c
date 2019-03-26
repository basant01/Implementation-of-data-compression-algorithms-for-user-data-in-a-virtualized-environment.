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
void CompressFile();
void ExpandFile();
void usage_exit();
void print_ratios();
long file_size();
unsigned int find_child_node();
unsigned int decode_string();
void fatal_error( char *fmt, ... );

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

void CompressFile(FILE *input, BIT_FILE *output, int argc, char *argv[] )
{
    int next_code;
    int character;
    int string_code;
    unsigned int index;
    unsigned int i;

    next_code = FIRST_CODE;
    for ( i = 0 ; i < TABLE_SIZE ; i++ )
        dict[ i ].code_value = UNUSED;
    if ( ( string_code = getc( input ) ) == EOF )
        string_code = END_OF_STREAM;
    while ( ( character = getc( input ) ) != EOF ) 
    {
        index = find_child_node( string_code, character );
        if ( dict[ index ].code_value != -1 )
            string_code = dict[ index ].code_value;
        else {
            if ( next_code <= MAX_CODE ) {
                dict[ index ].code_value = next_code++;
                dict[ index ].parent_code = string_code;
                dict[ index ].character = (char) character;
            }
            OutputBits( output, (unsigned long) string_code, BITS );
            string_code = character;
        }
    }
    OutputBits( output, (unsigned long) string_code, BITS );
    OutputBits( output, (unsigned long) END_OF_STREAM, BITS );
    while ( argc-- > 0 )
        printf( "Unknown argument: %s\n", *argv++ );
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

int main(int argc, char *argv[] )
{
    BIT_FILE *output;
    FILE *input;
    setbuf( stdout, NULL );
    /*if ( argc < 3 )
    usage_exit( argv[ 0 ] );*/
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
    printf( "Fatal error: " );
    vprintf( fmt, argptr );
    va_end( argptr );
    exit( -1 );
}