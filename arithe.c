#include <stdio.h>
#include <stdlib.h>
#include "errhand.h"
#include "bitio.h"
#include <string.h>
#include "main.h"
#include <stdarg.h>

#define PACIFIER_COUNT 2047



typedef struct {
unsigned short int low_count;
unsigned short int high_count;
unsigned short int scale;
} SYMBOL;



#ifdef __STDC__

void build_model( FILE *input, FILE *output );
void scale_counts( unsigned long counts[],unsigned char scaled_counts[] );
void build_totals( unsigned char scaled_counts[] );
void count_bytes( FILE *input, unsigned long counts[] );
void output_counts( FILE *output, unsigned char scaled_counts[] );
void input_counts( FILE *stream );
void convert_int_to_symbol( int symbol, SYMBOL *s );
void get_symbol_scale( SYMBOL *s );
int convert_symbol_to_int( int count, SYMBOL *s );
void initialize_arithmetic_encoder( void );
void encode_symbol( BIT_FILE *stream, SYMBOL *s );
void flush_arithmetic_encoder( BIT_FILE *stream );
short int get_current_count( SYMBOL *s );
void initialize_arithmetic_decoder( BIT_FILE *stream );
void remove_symbol_from_stream( BIT_FILE *stream, SYMBOL *s );

#else

void build_model();
void scale_counts();
void build_totals();
void count_bytes();
void output_counts();
void input_counts();
void convert_int_to_symbol();
void get_symbol_scale();
int convert_symbol_to_int();
void initialize_arithmetic_encoder();
void encode_symbol();
void flush_arithmetic_encoder();
short int get_current_count();
void initialize_arithmetic_decoder();
void remove_symbol_from_stream();

#endif

#define END_OF_STREAM 256
short int totals[ 258 ];

char*CompressionName = "Adaptive order 0 model with arithmetic coding";
char*Usage = "in-file out-file\n\n ";


void CompressFile( input, output, argc, argv )
FILE *input;
BIT_FILE *output;
char *argv[];
int argc;
{
int c;
SYMBOL s;

build_model( input, output->file );
initialize_arithmetic_encoder();

while ( ( c = getc( input ) )!= EOF ) {
convert_int_to_symbol( c, &s );
encode_symbol( output, &s );
}
convert_int_to_symbol( END_OF_STREAM, &s );
encode_symbol( output, &s );
flush_arithmetic_encoder( output );
OutputBits( output, 0L, 16 );
while ( argc-->0 ) {
  printf( "Unused argument: %s\n", *argv );
argv++;
}
}

void ExpandFile( input, output, argc, argv )
BIT_FILE *input;
FILE *output;
int argc;
char *argv[];
{
SYMBOL s;
int c;
 int count;

input_counts( input->file );
initialize_arithmetic_decoder( input );
for ( ; ; ) {
get_symbol_scale( &s );
count = get_current_count( &s );
c = convert_symbol_to_int( count, &s );
if ( c == END_OF_STREAM )
break;
remove_symbol_from_stream( input, &s );
putc( (char) c, output );
}
while ( argc--> 0 ) {
printf( "Unused argument: %s\n", *argv );

argv++;
}
}



void build_model( input, output )
FILE *input;
FILE *output;
{
unsigned long counts[ 256 ];
unsigned char scaled_counts[ 256 ];
count_bytes( input, counts );
scale_counts( counts, scaled_counts );
output_counts( output, scaled_counts );
build_totals( scaled_counts );
}



#ifndef SEEK_SET
#define SEEK_SET 0
#endif

void count_bytes( input, counts )
FILE *input;
unsigned long counts[];
{
long input_marker;
int i;
int c;
for ( i = 0 ; i < 256; i++ )
counts[ i ] = 0;
input_marker = ftell( input );
while ( ( c = getc( input ) ) != EOF )

counts[ c ]++;
fseek( input, input_marker, SEEK_SET );
}



void scale_counts( counts, scaled_counts )
unsigned long counts[];
unsigned char scaled_counts[];
{
int i;
unsigned long max_count;
unsigned int total;
unsigned long scale;

max_count = 0;
for ( i = 0 ; i < 256 ; i++ )
if ( counts[ i ] > max_count )
max_count = counts[ i ];
scale = max_count / 256;
scale = scale + 1;
for ( i = 0 ; i < 256 ; i++ ) {
scaled_counts[ i ] = (unsigned char ) ( counts[ i ] / scale );
if ( scaled_counts[ i ] == 0 && counts[ i ] != 0 )
scaled_counts[ i ] = 1;
}


total = 1;
for ( i = 0 ; i < 256 ; i++ )
total += scaled_counts[ i ];
if ( total > ( 32767 - 256 ) )
scale = 4;
else if ( total > 16383 )
scale = 2;
else
return;
for ( i = 0 ; i < 256 ; i++ )
scaled_counts[ i ] /= scale;
}



void build_totals( scaled_counts )
unsigned char scaled_counts[];
{
int i;
totals[ 0 ] = 0;
for ( i = 0 ; i < END_OF_STREAM ; i++ )
    totals[ i + 1 ] = totals[ i ] + scaled_counts[ i ];
totals[ END_OF_STREAM + 1 ] = totals[ END_OF_STREAM ] + 1;
}

void output_counts( output, scaled_counts )
FILE *output;
unsigned char scaled_counts[];
{
int first;
int last;
int next;
int i;

first = 0;
while ( first < 255 && scaled_counts[ first ] == 0 )
first++;



for ( ; first < 256 ; first = next ) {
last = first + 1;
for ( ; ; ) {
  for ( ; last < 256 ; last++ )
    if ( scaled_counts[ last ] == 0 )
break;
last-- ;
for ( next = last + 1; next < 256 ; next++ )
if ( scaled_counts[next] != 0 )
break;
if ( next > 255 )
break;
if ( ( next-last ) > 3 )
break;
last = next;
};

if ( putc( first, output ) != first )
fatal_error( "Error writing byte counts\n" );

if ( putc( last, output ) != last )
fatal_error( "Error writing byte counts\n" );

for ( i = first ; i <= last ; i++ ) {

if ( putc( scaled_counts[ i ], output ) != (int) scaled_counts[ i ] )
fatal_error( "Error writing byte counts\n" );

}
}
if ( putc( 0, output ) != 0 )
fatal_error( "Error writing byte counts\n" );

}

void input_counts( input )
FILE *input;
{
int first;
int last;
int i;
int c;
unsigned char scaled_counts[ 256 ];

for ( i = 0 ; i < 256 ; i++ )
scaled_counts[ i ] = 0;
if ( ( first = getc( input ) ) == EOF )
fatal_error( "Error reading byte counts\n" );


if ( ( last = getc( input ) ) == EOF )
fatal_error( "Error reading byte counts\n" );


for ( ; ; ) {
for ( i = first ; i <= last ; i++ )
if ( ( c = getc( input ) ) == EOF )
fatal_error( "Error reading byte counts\n" );


else
scaled_counts[ i ] = (unsigned char) c;
if ( ( first = getc( input ) ) == EOF )

fatal_error( "Error reading byte counts\n" );
if ( first == 0 )
break;
if ( ( last = getc( input ) ) == EOF )
fatal_error( "Error reading byte counts\n" );
}
build_totals( scaled_counts );
}


static unsigned short int code;
static unsigned short int low;
static unsigned short int high;
long underflow_bits;


void initialize_arithmetic_encoder() {
 low = 0;
 high = 0xffff;
underflow_bits = 0;
}


void flush_arithmetic_encoder( stream )
BIT_FILE* stream;
{

OutputBit( stream, low & 0x4000 );
underflow_bits++;
while ( underflow_bits--> 0 )
OutputBit( stream, ~low & 0x4000 );
}


void convert_int_to_symbol(  c, s )
int c;
SYMBOL *s;
{

s->scale = totals[ END_OF_STREAM + 1];
s->low_count = totals[ c ];
s->high_count = totals[ c + 1 ];
}



void get_symbol_scale( s )
SYMBOL *s;
{
s->scale = totals[ END_OF_STREAM + 1 ];
}


int convert_symbol_to_int( count, s )
int count;
SYMBOL *s;
{
int c;
for ( c = END_OF_STREAM ; count < totals[ c ] ; c-- );
s->high_count = totals[ c + 1 ];
s->low_count = totals[ c ];
return( c );
}

void encode_symbol( stream, s )
BIT_FILE *stream;
SYMBOL *s;
{
long range;

range = (long) ( high-low ) + 1;
high = low + (unsigned short int) (( range * s->high_count ) / s->scale - 1 );
low = low + (unsigned short int) (( range * s->low_count ) / s->scale );

for ( ; ; ) {

if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
OutputBit( stream, high & 0x8000 );
while ( underflow_bits > 0 ) {
OutputBit( stream, ~high & 0x8000 );
underflow_bits--;
}
}

else if ( ( low & 0x4000 ) && !( high & 0x4000 )) {
underflow_bits += 1;
low &= 0x3fff;
high |= 0x4000;
}
else
return ;
low <<= 1;
high <<= 1;
high |= 1;
}
}

short int get_current_count( s )
SYMBOL *s;
{
long range;
short int count;

range = (long) ( high - low ) + 1;
count = (short int)((((long) ( code - low ) + 1 ) * s->scale-1 ) / range ) ;
return( count );
}

void initialize_arithmetic_decoder( stream )
BIT_FILE *stream;
{
int i;
code = 0;
for ( i = 0 ; i < 16 ; i++ ) {
code <<= 1;
code += InputBit( stream );
}
low = 0;
high = 0xffff;
}

void remove_symbol_from_stream( stream, s )
BIT_FILE *stream;
SYMBOL *s;
{
long range;


range = (long)( high - low ) + 1;
high = low + (unsigned short int) (( range * s->high_count ) / s->scale - 1);
low = low + (unsigned short int)(( range * s->low_count ) / s->scale );

for ( ; ; ) {

if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
}

else if ((low & 0x4000) == 0x4000 && (high & 0x4000) == 0 ) {
code ^= 0x4000;
low &= 0x3ffff;
high |= 0x4000;
}
else

return;
low <<= 1;
high <<= 1;
high |= 1;
code <<= 1;
code += InputBit( stream );
}
}
#ifdef ___STDC___
void usage_exit( char *prog_name );
void print_ratios( char *input, char *output );
long file_size( char *name );
#else
void usage_exit();
void print_ratios();
long file_size();
#endif
int main( argc, argv )
int argc;
char *argv[];
{
BIT_FILE *output;
FILE *input;
setbuf( stdout, NULL );
if ( argc < 3 )
usage_exit( argv[ 0 ] );
input = fopen(argv [ 1 ], "rb" );
if ( input == NULL )
fatal_error( "Error opening %s for input\n", argv[ 1 ] );
output = OpenOutBitFile( argv[ 2 ] );
if ( output == NULL )
fatal_error( "Error opening %s for input\n", argv[ 2 ] );
printf( "\nCompressing %s to %s \n", argv[ 1 ], argv[ 2 ] );
printf( "Using %s\n", CompressionName );
CompressFile( input, output, argc-3, argv+3 );
CloseOutputBitFile( output );
fclose( input );
print_ratios( argv[ 1 ], argv[ 2 ] );
return( 0 );
}

void usage_exit( prog_name )
char *prog_name;
{
char *short_name;
char *extension;
short_name = strrchr( prog_name, '\\' );
if (short_name == NULL )
short_name = strrchr( prog_name, '/' );
if (short_name == NULL )
short_name = strrchr( prog_name, ':' );
if (short_name != NULL )
short_name++;
else
short_name = prog_name;
extension = strrchr( short_name, '.' );
if ( extension != NULL )
*extension = '\0';
printf( "\nUsage: %s %s\n", short_name, Usage );
exit( 0 );
}

#ifndef SEEK_END
#define SEEK_END 2
#endif
long file_size( name )
char *name;
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

void print_ratios( input, output )
char *input;
char *output;
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

BIT_FILE *OpenOutBitFile( name )
char *name;
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
BIT_FILE *OpenInBitFile( name )
char *name;
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
void CloseOutputBitFile( bit_file )
BIT_FILE *bit_file;
{
if ( bit_file->mask != 0x80 )
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
fatal_error( "Fatal error in CloseBitFile!\n" );
fclose( bit_file->file );
free( (char *) bit_file );
}
void CloseInputBitFile( bit_file )
BIT_FILE *bit_file;
{
fclose( bit_file->file );
free( (char*) bit_file );
}
void OutputBit( bit_file, bit )
BIT_FILE *bit_file;
int bit;
{
if ( bit )
bit_file->rack |= bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 ) {
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
//printf("Fatal error in OutputBit!\n" );
fatal_error( "Fatal error in OutputBit!\n" );
else
if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
putc( '.', stdout );
bit_file->rack = 0;
bit_file->mask = 0x80;
}
}
void OutputBits( bit_file, code, count )
BIT_FILE *bit_file;
unsigned long code;
int count;
{
unsigned long mask;

mask = 1L << ( count - 1 );
while ( mask != 0) {
if ( mask & code )
bit_file->rack |= bit_file->mask;
bit_file->mask >>= 1;
if ( bit_file->mask == 0 ) {
if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
fatal_error( "Fatal error in OutputBit!\n" );
else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT  ) == 0 )
putc( '.', stdout );
bit_file->rack = 0;
bit_file->mask = 0x80;
}
mask >>= 1;
}
}
int InputBit( bit_file )
BIT_FILE *bit_file;
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

unsigned long InputBits( bit_file, bit_count )
BIT_FILE *bit_file;
int bit_count;
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
void FilePrintBinary( file, code, bits )
FILE *file;
unsigned int code;
int bits;
{
unsigned int mask;
mask = 1 << ( bits - 1 );
while ( mask != 0 ){
if ( code & mask )
fputc( '1', file );
else
fputc( '0', file);
mask >>= 1;
}
}







#ifdef __STDC__
void fatal_error( char *fmt, ... )
#else
#ifdef __UNIX__
void fatal_error( fmt, va_alist )
char *fmt;
va_dcl
#else
void fatal_error( fmt )
char *fmt;
#endif
#endif
{
    va_list argptr;

    va_start( argptr, fmt );
    printf( "Fatal error: " );
    vprintf( fmt, argptr );
    va_end( argptr );
    exit( -1 );
}
