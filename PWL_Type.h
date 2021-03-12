/*********************** SETIS - Automa��o e Sistemas ************************

 Arquivo          : PWL_Type.h
 Projeto          : Pay&Go WEB
 Plataforma       : Linux
 Data de cria��o  : 20/02/2014
 Autor            : Guilherme Eduardo Leite
 Descri��o        : Defini��es dos tipos utiliza na plataforma Linux pela
                    dll para integra��o com a solu��o Pay&Go WEB.
 ================================= HIST�RICO =================================
 Data      Respons�vel Modifica��o
 --------  ----------- -------------------------------------------------------
\*****************************************************************************/

#ifndef _PWLTYPE_INCLUDED_
#define _PWLTYPE_INCLUDED_

// Tipos b�sicos utilizados
#ifndef PW_EXPORT 
#define PW_EXPORT
#endif /* PW_EXPORT */

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   1
#endif

typedef char           Int8;
typedef unsigned char  Uint8;

typedef short          Int16;
typedef unsigned short Uint16;

typedef long           Int32;
typedef unsigned long  Uint32;

typedef unsigned char  Byte;

typedef unsigned short Word;
typedef unsigned int   Dword;

typedef int            Bool;


// Valor m�ximo que pode ser atribu�do a um tipo Int16
#ifndef MAXINT16
#define MAXINT16        32767
#endif /* MAXINT16 */

#endif

