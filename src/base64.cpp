/*---------------------------------------------------------------------------*/
/* base64                                                                    */
/* ======                                                                    */
/*                                                                           */
/* Base64 is a stand-alone C program to encode 7-Bit ASCII strings into      */
/* base64 encoded strings and decode base64 encoded strings back into 7 bit  */
/* ASCII strings.                                                            */
/*                                                                           */
/* Base64 encoding is sometimes used for simple HTTP authentication. That is */
/* when strong encryption isn't necessary, Base64 encryption is used to      */
/* authenticate User-ID's and Passwords.                                     */
/*                                                                           */
/* Base64 processes a string by octets (3 Byte blocks).  For every octet in  */
/* the decoded string, four byte blocks are generated in the encoded string. */
/* If the decoded string length is not a multiple of 3, the Base64 algorithm */
/* pads the end of the encoded string with equal signs '='.                  */
/*                                                                           */
/* An example taken from RFC 2617 (HTTP Authentication):                     */
/*                                                                           */
/* Resource (URL) requires basic authentication (Authorization: Basic) for   */
/* access, otherwise a HTTP 401 Unauthorized response is returned.           */
/*                                                                           */
/* User-ID:Password string  = "Aladdin:open sesame"                          */
/* Base64 encoded   string  = "QWxhZGRpbjpvcGVuIHNlc2FtZQ=="                 */
/*                                                                           */
/* Usage:   base64 OPTION [STRING]                                           */
/* ------                                                                    */
/* OPTION:  -h Displays a brief messages.                                    */
/*          -e Base64 encode the 7-Bit ASCII STRING.                         */
/*          -d Decode the Base64 STRING to 7-Bit ASCII.                      */
/*                                                                           */
/* STRING:  Either a 7-Bit ASCII text string for encoding or a Base64        */
/*          encoded string for decoding back to 7-Bit ASCII.                 */
/*                                                                           */
/* Note:    For EBCDIC and other collating sequences, the STRING must first  */
/*          be converted to 7-Bit ASCII before passing it to this module and */
/*          the return string must be converted back to the appropriate      */
/*          collating sequence.                                              */
/*                                                                           */
/* Student Exercises:                                                        */
/* ------------------                                                        */
/* 1. Modify base64 to accept an additional parameter "Quiet Mode" (-q) to   */
/*    optionally supress the ending statistics and only display the encoded  */
/*    or decoded string.                                                     */
/*                                                                           */
/* 2. Make base64 callable from another program as follows:                  */
/*    a. Add an externally callable function to determine and return the     */
/*       size of the buffer required for encoding or decoding.               */
/*    b. Make base64 accept three parameters; input and output buffer point- */
/*       ers and a flag for indicate encoding or decoding.                   */
/*    c. Modify base64 so that a calling program can:                        */
/*       i.   Request the size of a buffer required either for encoding or   */
/*            decoding.                                                      */
/*       ii.  Allocate a buffer based on the result from the previous        */
/*            call.                                                          */
/*       iii. Call base64 with the appropriate pointers and flag to encode   */
/*            or decode a string into the callers buffer.                    */
/*                                                                           */
/* Copyright (c) 1994 - 2001                                                 */
/* Marc Niegowski                                                            */
/* Connectivity, Inc.                                                        */
/* All rights reserved.                                                      */
/*---------------------------------------------------------------------------*/
//#include    <stdlib.h>                      // calloc and free prototypes.
//#include    <stdio.h>                       // printf prototype.
//#include    <string.h>                      // str* and memset prototypes.

#include "bnbt.h"
#include "base64.h"

typedef
unsigned
char    uchar;                              // Define unsigned char as uchar.

typedef
unsigned
int     uint;                               // Define unsigned int as uint.

bool    b64valid(uchar *);                  // Tests for a valid Base64 char.
char   *b64isnot(char *);                   // Displays an invalid message.
char   *b64buffer(const char *, bool);      // Alloc. encoding/decoding buffer.

// Macro definitions:

#define b64is7bit(c)  ((c) > 0x7f ? 0 : 1)  // Valid 7-Bit ASCII character?
#define b64blocks(l) (((l) + 2) / 3 * 4 + 1)// Length rounded to 4 byte block.
#define b64octets(l)  ((l) / 4  * 3 + 1)    // Length rounded to 3 byte octet. 

// Note:    Tables are in hex to support different collating sequences

static
const                                       // Base64 encoding and decoding
uchar   pBase64[]   =   {                   // table.
                        0x3e, 0x7f, 0x7f, 0x7f, 0x3f, 0x34, 0x35, 0x36,
                        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x7f,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x01,
                        0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
                        0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x1a, 0x1b,
                        0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
                        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
                        };

/*---------------------------------------------------------------------------*/
/* b64decode - Decode a Base64 string to a 7-Bit ASCII string.               */
/* ===========================================================               */
/*                                                                           */
/* Call with:   char *  - The Base64 string to decode.                       */
/*                                                                           */
/* Returns:     bool    - True (!0) if the operation was successful.         */
/*                        False (0) if the operation was unsuccessful.       */
/*---------------------------------------------------------------------------*/
char *b64decode(const char *s)
{
    int l = (int)strlen(s);             // Get length of Base64 string.
    char   *b;                              // Decoding buffer pointers.
    uchar   c = 0;                          // Character to decode.
    int     x = 0;                          // General purpose integers.
    int     y = 0;

    static                                  // Collating sequence...
    const                                   // ...independant "===".
    char    pPad[]  =   {0x3d, 0x3d, 0x3d, 0x00};

    if  (l % 4)                             // If it's not modulo 4, then it...
        return b64isnot(0);              // ...can't be a Base64 string.

	b = strchr((char *)s, pPad[0]);
    if  ((b) != 0)						// Only one, two or three equal...
    {                                       // ...'=' signs are allowed at...
        if  ((b - s) < (l - 3))             // ...the end of the Base64 string.
            return b64isnot(0);          // Any other equal '=' signs are...
        else                                // ...invalid.
            if  (strncmp(b, (const char *) pPad + 3 - (s + l - b), s + l - b))
                return b64isnot(0);
    }

	b = b64buffer(s, false);
    if  (!(b))							// Allocate a decoding buffer.
        return 0;                        // Can't allocate decoding buffer.

    x = 0;                                  // Initialize index.

    while ((c = *s++))                      // Decode every byte of the...
    {                                       // Base64 string.
        if  (c == pPad[0])                  // Ignore "=".
            break;

        if (!b64valid(&c))                  // Valid Base64 Index?
            return b64isnot(b);             // No, return false.
        
        switch(x % 4)                       // Decode 4 byte words into...
        {                                   // ...3 byte octets.
        case    0:                          // Byte 0 of word.
            b[y]    =  c << 2;
            break;                          
        case    1:                          // Byte 1 of word.
            b[y]   |=  c >> 4;

            if (!b64is7bit((uchar) b[y++])) // Is 1st byte of octet valid?
                return b64isnot(b);         // No, return false.

            b[y]    = (c & 0x0f) << 4;
            break;
        case    2:                          // Byte 2 of word.
            b[y]   |=  c >> 2;

            if (!b64is7bit((uchar) b[y++])) // Is 2nd byte of octet valid?
                return b64isnot(b);         // No, return false.

            b[y]    = (c & 0x03) << 6;
            break;
        case    3:                          // Byte 3 of word.
            b[y]   |=  c;

            if (!b64is7bit((uchar) b[y++])) // Is 3rd byte of octet valid?
                return b64isnot(b);         // No, return false.
        }
        x++;                                // Increment word byte.
    }

	return b;
}

/*---------------------------------------------------------------------------*/
/* b64valid - validate the character to decode.                              */
/* ============================================                              */
/*                                                                           */
/* Checks whether the character to decode falls within the boundaries of the */
/* Base64 decoding table.                                                    */
/*                                                                           */
/* Call with:   char    - The Base64 character to decode.                    */
/*                                                                           */
/* Returns:     bool    - True (!0) if the character is valid.               */
/*                        False (0) if the character is not valid.           */
/*---------------------------------------------------------------------------*/
bool b64valid(uchar *c)
{
    if ((*c < 0x2b) || (*c > 0x7a))         // If not within the range of...
        return false;                       // ...the table, return false.
    
    if ((*c = pBase64[*c - 0x2b]) == 0x7f)  // If it falls within one of...
        return false;                       // ...the gaps, return false.

    return true;                            // Otherwise, return true.
}

/*---------------------------------------------------------------------------*/
/* b64isnot - Display an error message and clean up.                         */
/* =================================================                         */
/*                                                                           */
/* Call this routine to display a message indicating that the string being   */
/* decoded is an invalid Base64 string and de-allocate the decoding buffer.  */
/*                                                                           */
/* Call with:   char *  - Pointer to the Base64 string being decoded.        */
/*              char *  - Pointer to the decoding buffer or 0 if it isn't */
/*                        allocated and doesn't need to be de-allocated.     */
/*                                                                           */
/* Returns:     bool    - True (!0) if the character is valid.               */
/*                        False (0) if the character is not valid.           */
/*---------------------------------------------------------------------------*/
char *b64isnot(char *b)
{
    if  (b)                                 // If the buffer pointer is not...
        free(b);                            // ...0, de-allocate it.

    return  0;
}

/*---------------------------------------------------------------------------*/
/* b64buffer - Allocate the decoding or encoding buffer.                     */
/* =====================================================                     */
/*                                                                           */
/* Call this routine to allocate an encoding buffer in 4 byte blocks or a    */
/* decoding buffer in 3 byte octets.  We use "calloc" to initialize the      */
/* buffer to 0x00's for strings.                                             */
/*                                                                           */
/* Call with:   char *  - Pointer to the string to be encoded or decoded.    */
/*              bool    - True (!0) to allocate an encoding buffer.          */
/*                        False (0) to allocate a decoding buffer.           */
/*                                                                           */
/* Returns:     char *  - Pointer to the buffer or 0 if the buffer        */
/*                        could not be allocated.                            */
/*---------------------------------------------------------------------------*/
char *b64buffer(const char *s, bool f)
{
    int     l = (int)strlen(s);                  // String size to encode or decode.

    if  (!l)                                // If the string size is 0...
        return  0;                       // ...return 0.

    return (char *)calloc((f ? b64blocks(l) : b64octets(l)),sizeof(char));
}
