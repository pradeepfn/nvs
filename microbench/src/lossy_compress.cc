#include <iostream>
#include <zlib.h>
#include <bzlib.h>
#include <sys/time.h>
#include <cassert>

using namespace std;

/*
 *   Extract the first the 3 bytes, from double value
 */
uint64_t split_bytes (double value)
{
    uint64_t mask = 0xFFFFFC0000000000;
    uint64_t val;

    assert (sizeof (double) == sizeof (uint64_t));

    memcpy (&val, &value, sizeof (double));
    val = val & mask;

    return val;
}

/*
 *   Extract i^th byte from 64-bit double.
 */
char get_byte (uint64_t value, int i)
{
    unsigned char byte[8];
    memcpy(byte, &value , 8) ;

    return byte[i];
}

/*
 *   Clock function
 */
double dclock(void)
{
    struct timeval tv;
    gettimeofday(&tv,0);
    return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
}

/*
 *  Compress a stream of double precision numbers.
 *
 *  PARAMETERS:
 *
 *  input - Double precision input array
 *  input_size - Total size of above input array in bytes
 *  output_size - Size of the compressed input
 *
 *  RETURN VALUE:
 *  void * - compressed array
 */
void *compress (double *input, uint64_t input_size, uint64_t &output_size)
{
    uint32_t nelements = input_size / sizeof (double);
    unsigned char *vertical = (unsigned char *) malloc (nelements * 3);
    int cnt = 0 ;

    if (!vertical) {
        fprintf (stderr, "[%s: %d] Unable to allocate memory\n", __FUNCTION__, __LINE__);
    }

    /*
     * Align first 3 bytes vertically
     * (Byte 7, 6, 5 are the first 3 significant bytes in Little-Endian Format)
     */
    for(uint32_t i = 0; i < nelements; i ++) {
        uint64_t data = split_bytes (input [i]);
        vertical [i] =  get_byte (data, 7);
        vertical [i + 1 * nelements] = get_byte (data, 6);
        vertical [i + 2 * nelements] = get_byte (data, 5);
    }

    uLongf original_size = nelements * 3;
    uLongf compressed_size = original_size;

    /*
     * Compress using Zlib
     */
    unsigned char *compressed_output = (unsigned char *) malloc (compressed_size);
    if (!compressed_output) {
        fprintf (stderr, "[%s: %d] Unable to allocate memory\n", __FUNCTION__, __LINE__);
    }
    int bzerr = compress2 ((Bytef*) compressed_output, &compressed_size, (Bytef*) vertical, original_size, Z_DEFAULT_COMPRESSION);

    free (vertical);

    if (bzerr != Z_OK) {
        fprintf (stderr, "[%s: %d] Compression failed", __FUNCTION__, __LINE__);
        output_size = 0;
        return NULL;
    }

    output_size = compressed_size;
    return compressed_output;
}

/*
 *  Decompress the compressed numbers
 *
 *  PARAMETERS:
 *
 *  compressed_input - Data compressed using the above compress function
 *  input_size - Size of the original input array in bytes
 *  output_size - Size of the compressed input in bytes
 *
 *  RETURN VALUE:
 *  double * - Uncompressed double precision array
 */
double *decompress (void *compressed_input, uint64_t input_size, uint64_t compressed_input_size)
{
    // Total number of elements
    int nelements = input_size / sizeof (double);
    int decompressed_input_size = nelements * 3;
    unsigned char *vertical = (unsigned char *) malloc (decompressed_input_size);

    if (!vertical) {
        fprintf (stderr, "[%s: %d] Unable to allocate memory\n", __FUNCTION__, __LINE__);
    }

    double *output = (double *) malloc (input_size);

    if (!output) {
        fprintf (stderr, "[%s: %d] Unable to allocate memory\n", __FUNCTION__, __LINE__);
    }

    /*
     * Perform decompression
     */
    uLong dc_size = decompressed_input_size;
    int bzerr = uncompress ((Bytef *) vertical, &dc_size, (Bytef *) compressed_input, compressed_input_size);

    if (bzerr != Z_OK) {
        free (vertical);
        free (output);
        fprintf (stderr, "[%s: %d] Decompression failed\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    /*
     * Restore bytes to original location
     * (Byte 7, 6, 5 are the first 3 significant bytes in Little-Endian Format)
     */
    for(int j = 0; j < nelements; j ++)
    {
        unsigned char *data = (unsigned char *) &output [j];
        memcpy (data + 7, &vertical [j], sizeof (unsigned char));
        memcpy (data + 6, &vertical [j + nelements * 1], sizeof (unsigned char));
        memcpy (data + 5, &vertical [j + nelements * 2], sizeof (unsigned char));
    }

    free (vertical);

    return output;
}

void test_codec (char filename[], char N[])
{
    int nelements;
    sscanf (N, "%d", &nelements);

    FILE *fp = fopen (filename, "rb");
    double data [nelements];

    if (!fp) {
        fprintf (stdout, "[%s: %d] Unable to open file: %s\n", __FUNCTION__, __LINE__, filename);
        return ;
    }

    fread (data, sizeof (double), nelements, fp);
    fclose (fp);

    uint64_t compressed_size;

    // Test compression
    double start_c = dclock ();
    void *compressed_data = compress (data, nelements * sizeof (double), compressed_size);
    double end_c = dclock ();

    // Test decompression
    double start_d = dclock ();
    double *decompressed_data = decompress (compressed_data, nelements * sizeof (double), compressed_size);
    double end_d = dclock ();

    fprintf (stdout, "Compression ratio: %lf\n", compressed_size * 1.0 / (nelements * sizeof (double)) * 100);
    fprintf (stdout, "Total compression time: %lf seconds\n", end_c - start_c);
    fprintf (stdout, "Total decompression time: %lf seconds\n", end_d - start_d);

    // Check relative error
    for (int i = 0; i < nelements; i ++)
    {
        if ((data [i] - decompressed_data [i]) / data [i] * 100 > 0.1) {
            fprintf (stdout, "Relative error > 0.1\n");
            break;
        }
    }

    free (compressed_data);
    free (decompressed_data);

    return;
}

int main (int argc, char *argv[])
{

    if (argc == 3) {
        // argv [1] - Input file name
        // argv [2] - Total number of double elements
        test_codec (argv [1], argv [2]);
    }

    return 1;
}
