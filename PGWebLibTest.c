/*********************** SETIS - Automação e Sistemas *********************************************

 Arquivo          : PGWebLibTest.c
 Projeto          : Pay&Go WEB
 Plataforma       : Linux
 Data de criação  : 19/02/2014
 Autor            : Guilherme Eduardo Leite
 Descrição        : Console de testes extendido para a simulação de um cliente
                    para a so de integração com a solução Pay&Go WEB.
 ================================= HISTÓRICO ======================================================
 Data      Responsável Modificação
 --------  ----------- ----------------------------------------------------------------------------
 29/05/20  Guilherme   - Alteração da forma de captura de dados em PWDAT_MENU, de forma que possa 
                         receber teclas de atalho com mais de 1 dígito.
                       - Alteração na forma de captura de dado de cartão digitado em relação ao 
                         primeiro dígito inserido.
                       - Criação da função TesteRemoveCard para simular uma transação com a remoção 
                         do cartão feita pela automação (CA19-0093).
                       - Acrescentado função iGetStringEx para captura de dados com tamanho limite. 
                         Alterado o fluxo de venda para captura do valor da transação e inseire o
                         código e expoente da moeda antes da inicialização da transação com a 
                         biblioteca.
                       - Acrescentado tag PWINFO_TRNORIGLOCREF (CA19-0123).
                       - Acrescentado tratamento quando recebido captura de dado PWDAT_USERAUTH 
                         (CA19-0162).
                       - Adicionado GetOperationsEx.
                       - Só solicita valor da venda quando a biblioteca já está inicializada. 
                       - Passa a salvar dados da confirmação em arquivo, para eventual queda de 
                         energia e solicitar a confirmação/desfazimento da transação ao final do 
                         teste de venda.
                       - No caso da execução de menus com somente uma opção, caso a opção default 
                         seja ela mesma, escolhe automaticamente, senão, exibe a opção única para 
                         confirmação.
                       - Criado condição para ser possível se pressionar a tecla ESC e cancelar 
                         a operação quando retornado PWRET_NOTHING.
                       - Implementação da confirmação positiva no PIN-pad.
                       - Remoção de warnings para x64.
15/Jun/20  Mateus V.   - Corrige captura de menu para menus com mais de 20 itens.  
02/Jul/20  Guilherme   - Inclusão da importação da função pPW_iPPPositiveConfirmation(CA20-128).
10/Dez/20  Guilherme   - Acrescentndo AUTCAP_DSPQRCODE nas capacidades da automação.
                       - Pula a exibição de PWINFO_AUTHPOSQRCODE em PrintResultParams.
                       - Passa a verificar a necessidade de confirmação para transações canceladas 
                         pelo usuário durante uma captura de dados.
                       - Implementada a captura PWDAT_DSPQRCODE, exibindo a string com o QR-code na tela.
                       - Incluído na função AddMandatoryParams um exemplo de como a automação deve indicar
                         que quer receber a string com o QR-code para exibição na seu próprio display.
\*************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio_ext.h>
#include "PGWebLib.h"

// Definição da versão do aplicativo
#define  PGWEBLIBTEST_VERSION "1.5.0.4"

// Estrutura para importação das funções da DLL "PGWebLib.dll"
typedef struct {
    Int16 (PW_EXPORT *pPW_iInit)              (const char* pszWorkingDir);
    Int16 (PW_EXPORT *pPW_iNewTransac)        (Int16 iOper);
    Int16 (PW_EXPORT *pPW_iAddParam)          (Int16 iParam, const char *szValue);
    Int16 (PW_EXPORT *pPW_iExecTransac)       (PW_GetData vstParam[], Int16 *piNumParam);
    Int16 (PW_EXPORT *pPW_iGetResult)         (Int16 iInfo, char *pszData, Uint32 ulDataSize);
    Int16 (PW_EXPORT *pPW_iConfirmation)      (Uint32 ulResult, const char* pszReqNum, const char* pszLocRef, const char* pszExtRef,
                                               const char* pszVirtMerch, const char* pszAuthSyst);
    Int16 (PW_EXPORT *pPW_iIdleProc)          (void);
    Int16 (PW_EXPORT *pPW_iPPAbort)           (void);
    Int16 (PW_EXPORT *pPW_iPPEventLoop)       (char *pszDisplay, Uint32 ulDisplaySize);
    Int16 (PW_EXPORT *pPW_iPPGetCard)         (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPGetPIN)          (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPGetData)         (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPGoOnChip)        (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPFinishChip)      (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPConfirmData)     (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPPositiveConfirmation)     (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iPPRemoveCard)      (void);
    Int16 (PW_EXPORT *pPW_iPPDisplay)         (const char* pszMsg);
    Int16 (PW_EXPORT *pPW_iPPWaitEvent)       (Uint32 *pulEvent);
    Int16 (PW_EXPORT *pPW_iGetOperations)     (Byte bOperType, PW_Operations vstOperations[], Int16 *piNumOperations);
    Int16 (PW_EXPORT *pPW_iGetOperationsEx)   (Byte bOperType, PW_OperationsEx vstOperations[], Int16 iStructSize, Int16 *piNumOperations);
    Int16 (PW_EXPORT *pPW_iPPGenericCMD)      (Uint16 uiIndex);
    Int16 (PW_EXPORT *pPW_iTransactionInquiry)(const char *pszXmlRequest, char* pszXmlResponse, Uint32 ulXmlResponseLen);
    Int16 (PW_EXPORT *pPW_iPPGetUserData)     (Uint16 uiMessageId, Byte bMinLen, Byte bMaxLen, Int16 iToutSec, char *pszData);
    Int16 (PW_EXPORT *pPW_iPPGetPINBlock)     (Byte bKeyID, const char* pszWorkingKey, Byte bMinLen, Byte bMaxLen, Int16 iToutSec, 
                                               const char* pszPrompt, char* pszData);
    Int16 (PW_EXPORT *pPW_iPPTestKey)         (Uint16 uiIndex);
} HwFuncs;

// Estrutura para armazenamento de dados para confirmação de transação
typedef struct {
   char szReqNum[11];
   char szHostRef[51];
   char szLocRef[51];
   char szVirtMerch[19];
   char szAuthSyst[21];
} ConfirmData;

// Variáveis globais
static void*         ghHwLib;
static HwFuncs       gstHwFuncs;
static ConfirmData   gstConfirmData;
static Bool          gfAutoAtendimento;
static const char    *gszUserPassword = "1111";
static const char    *gszTechPassword = "314159";
static const char    *gszConfirmFileName = "_confirmation";

// Capacidades da Automação (soma dos valores abaixo):
#define AUTCAP_TROCO          1     /* funcionalidade de troco/saque */
#define AUTCAP_DESCONTO       2     /* funcionalidade de desconto */
#define AUTCAP_FIXO           4     /* valor fixo, sempre incluir */
#define AUTCAP_CUPOMDIF       8     /* impressão das vias diferenciadas do comprovante para Cliente/Estabelecimento */
#define AUTCAP_CUPOMRED       16    /* impressão do cupom reduzido */
#define AUTCAP_SALDOVOUCHER   32    /* utilização de saldo total do voucher para abatimento do valor da compra */
#define AUTCAP_REMOCAOCARTAO  64    /* tratar a remoção do cartão do PIN-pad */
#define AUTCAP_DSPCHECKOT     128   /* Capacidade de exibição de mensagens durante o fluxo transacional */
#define AUTCAP_DSPQRCODE      256   /* Capacidade de exibição de QR Code no checkout. */

// Definições para possíveis modo de entrada do cartão
#define CARTAO_DIGITADO       1     /* digitado */
#define CARTAO_MAGNETICO      2     /* tarja magnética */
#define CARTAO_CHIP           4     /* chip com contato */
#define CARTAO_FBMAGNETICO    16    /* fallback de chip para tarja */
#define CARTAO_MAGNETICOCTLS  32    /* chip sem contato */
#define CARTAO_CHIPCTLS       64    /* sem contato simulando tarja */
#define CARTAO_FBDIGITADO     256   /* fallback de tarja para digitado */

// Capacidades da automação tratadas por todas transaçães ao aplicativo de testes
#define PGWLT_AUTCAP  AUTCAP_TROCO|AUTCAP_DESCONTO|AUTCAP_FIXO|AUTCAP_CUPOMDIF|AUTCAP_DSPCHECKOT|AUTCAP_DSPQRCODE

#define GETS_FIXEDLEN   0x0001  // Entrada tem tamanho fixo (= iMaxChar).
#define GETS_NODEFAULT  0x0002  // Não há valor default (pszData é desprezado como entrada).
#define GETS_NOSZ       0x0004  // pszData não leva terminador NUL na saída.
#define GETS_ACCEPTBLK  0x0008  // Aceita o campo vazio (para uso em combinação com GETS_FIXEDLEN).
#define GETS_EVEN       0x0010  // Só aceita entrada com número par de caracteres
#define GETS_ASCCR      0x0020  // Mostra mensagem "(ASC, #=CR)" e aceita "#" como CR.
#define CONSOLE_WIDTH 80

/******************************************/
/* FUNÇÕES LOCAIS AUXILIARES              */
/******************************************/
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

/*=====================================================================================*\
 Funcao     :  InputCR

 Descricao  :  Esta função é utilizada para substituir o caractere utilizado pelo 
               Pay&Go Web para a quebra de linha ('\r') para o padrão utilizado 
               pelos aplicativos console Linux ("\r\n").
 
 Entradas   :  pszSourceStr   :  String a ser alterada.

 Saidas     :  pszSourceStr   :  String com o caractere de quebra de linha substituído.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void InputCR(char* pszSourceStr)
{
   int i, j, iSourceLen;
   char cAux, cTemp;

   iSourceLen = strlen(pszSourceStr);

   for( i = 0; i < iSourceLen; i++)
   {
      if( pszSourceStr[i] == '\r' && pszSourceStr[i+1] != '\n')
      {
         cAux = pszSourceStr[i+1];
         pszSourceStr[i+1] = '\n';
         for( j = i+1; j <= iSourceLen; j++)
         {
            cTemp = pszSourceStr[j+1];
            pszSourceStr[j+1] = cAux;
            cAux = cTemp;
         }
         iSourceLen++;
      }
   }
}

/*=====================================================================================*\
 Funcao     :  iConfirmaTransacao

 Descricao  :  Responsavel por enviar a confirmação.
 
 Entradas   :  ulCause: Codigo da causa da confirmação.

 Saidas     :  Nao ha.
 
 Retorno    :  PWRET_XX
\*=====================================================================================*/
static Int16 iConfirmaTransacao (Uint32 ulCause) 
{
   Int16 iRet;

   iRet = gstHwFuncs.pPW_iConfirmation(ulCause, gstConfirmData.szReqNum, 
      gstConfirmData.szLocRef, gstConfirmData.szHostRef, gstConfirmData.szVirtMerch, 
      gstConfirmData.szAuthSyst);
   memset(&gstConfirmData, 0, sizeof(gstConfirmData));

   return iRet;
}
/*=====================================================================================*\
 Funcao     :  pszGetInfoDescription

 Descricao  :  Esta função recebe um código PWINFO_XXX e retorna uma string com a 
               descrição da informação representada por aquele código.
 
 Entradas   :  wIdentificador :  Código da informação (PWINFO_XXX).

 Saidas     :  nao ha.
 
 Retorno    :  String representando o código recebido como parâmetro.
\*=====================================================================================*/
static const char* pszGetInfoDescription(Int16 wIdentificador)
{
   switch(wIdentificador)
   {
      case(PWINFO_OPERATION        ): return "PWINFO_OPERATION";
      case(PWINFO_POSID            ): return "PWINFO_POSID";
      case(PWINFO_AUTNAME          ): return "PWINFO_AUTNAME";
      case(PWINFO_AUTVER           ): return "PWINFO_AUTVER";
      case(PWINFO_AUTDEV           ): return "PWINFO_AUTDEV";
      case(PWINFO_DESTTCPIP        ): return "PWINFO_DESTTCPIP";
      case(PWINFO_MERCHCNPJCPF     ): return "PWINFO_MERCHCNPJCPF";
      case(PWINFO_AUTCAP           ): return "PWINFO_AUTCAP";
      case(PWINFO_TOTAMNT          ): return "PWINFO_TOTAMNT";
      case(PWINFO_CURRENCY         ): return "PWINFO_CURRENCY";
      case(PWINFO_CURREXP          ): return "PWINFO_CURREXP";
      case(PWINFO_FISCALREF        ): return "PWINFO_FISCALREF";
      case(PWINFO_CARDTYPE         ): return "PWINFO_CARDTYPE";
      case(PWINFO_PRODUCTNAME      ): return "PWINFO_PRODUCTNAME";
      case(PWINFO_DATETIME         ): return "PWINFO_DATETIME";
      case(PWINFO_REQNUM           ): return "PWINFO_REQNUM";
      case(PWINFO_AUTHSYST         ): return "PWINFO_AUTHSYST";
      case(PWINFO_VIRTMERCH        ): return "PWINFO_VIRTMERCH";
      case(PWINFO_AUTMERCHID       ): return "PWINFO_AUTMERCHID";
      case(PWINFO_PHONEFULLNO      ): return "PWINFO_PHONEFULLNO";
      case(PWINFO_FINTYPE          ): return "PWINFO_FINTYPE";
      case(PWINFO_INSTALLMENTS     ): return "PWINFO_INSTALLMENTS";
      case(PWINFO_INSTALLMDATE     ): return "PWINFO_INSTALLMDATE";
      case(PWINFO_PRODUCTID        ): return "PWINFO_PRODUCTID";
      case(PWINFO_RESULTMSG        ): return "PWINFO_RESULTMSG";
      case(PWINFO_CNFREQ           ): return "PWINFO_CNFREQ";
      case(PWINFO_AUTLOCREF        ): return "PWINFO_AUTLOCREF";
      case(PWINFO_AUTEXTREF        ): return "PWINFO_AUTEXTREF";
      case(PWINFO_AUTHCODE         ): return "PWINFO_AUTHCODE";
      case(PWINFO_AUTRESPCODE      ): return "PWINFO_AUTRESPCODE";
      case(PWINFO_DISCOUNTAMT      ): return "PWINFO_DISCOUNTAMT";
      case(PWINFO_CASHBACKAMT      ): return "PWINFO_CASHBACKAMT";
      case(PWINFO_CARDNAME         ): return "PWINFO_CARDNAME";
      case(PWINFO_ONOFF            ): return "PWINFO_ONOFF";
      case(PWINFO_BOARDINGTAX      ): return "PWINFO_BOARDINGTAX";
      case(PWINFO_TIPAMOUNT        ): return "PWINFO_TIPAMOUNT";
      case(PWINFO_INSTALLM1AMT     ): return "PWINFO_INSTALLM1AMT";
      case(PWINFO_INSTALLMAMNT     ): return "PWINFO_INSTALLMAMNT";
      case(PWINFO_RCPTFULL         ): return "PWINFO_RCPTFULL";
      case(PWINFO_RCPTMERCH        ): return "PWINFO_RCPTMERCH";
      case(PWINFO_RCPTCHOLDER      ): return "PWINFO_RCPTCHOLDER";
      case(PWINFO_RCPTCHSHORT      ): return "PWINFO_RCPTCHSHORT";
      case(PWINFO_TRNORIGDATE      ): return "PWINFO_TRNORIGDATE";
      case(PWINFO_TRNORIGNSU       ): return "PWINFO_TRNORIGNSU";
      case(PWINFO_TRNORIGAMNT      ): return "PWINFO_TRNORIGAMNT";
      case(PWINFO_TRNORIGAUTH      ): return "PWINFO_TRNORIGAUTH";
      case(PWINFO_TRNORIGREQNUM    ): return "PWINFO_TRNORIGREQNUM";
      case(PWINFO_TRNORIGTIME      ): return "PWINFO_TRNORIGTIME";
      case(PWINFO_CARDFULLPAN      ): return "PWINFO_CARDFULLPAN";
      case(PWINFO_CARDEXPDATE      ): return "PWINFO_CARDEXPDATE";
      case(PWINFO_CARDNAMESTD      ): return "PWINFO_CARDNAMESTD";
      case(PWINFO_CARDPARCPAN      ): return "PWINFO_CARDPARCPAN";
      case(PWINFO_BARCODENTMODE    ): return "PWINFO_BARCODENTMODE";
      case(PWINFO_BARCODE          ): return "PWINFO_BARCODE";
      case(PWINFO_MERCHADDDATA1    ): return "PWINFO_MERCHADDDATA1";
      case(PWINFO_MERCHADDDATA2    ): return "PWINFO_MERCHADDDATA2";
      case(PWINFO_MERCHADDDATA3    ): return "PWINFO_MERCHADDDATA3";
      case(PWINFO_MERCHADDDATA4    ): return "PWINFO_MERCHADDDATA4";
      case(PWINFO_PAYMNTTYPE       ): return "PWINFO_PAYMNTTYPE";
      case(PWINFO_USINGPINPAD      ): return "PWINFO_USINGPINPAD";
      case(PWINFO_PPCOMMPORT       ): return "PWINFO_PPCOMMPORT";
      case(PWINFO_IDLEPROCTIME     ): return "PWINFO_IDLEPROCTIME";
      case(PWINFO_PNDAUTHSYST		  ): return "PWINFO_PNDAUTHSYST";	      
      case(PWINFO_PNDVIRTMERCH     ): return "PWINFO_PNDVIRTMERCH";
      case(PWINFO_PNDREQNUM        ): return "PWINFO_PNDREQNUM";   
      case(PWINFO_PNDAUTLOCREF     ): return "PWINFO_PNDAUTLOCREF";
      case(PWINFO_PNDAUTEXTREF     ): return "PWINFO_PNDAUTEXTREF";
      case(PWINFO_WALLETUSERIDTYPE ): return "PWINFO_WALLETUSERIDTYPE";
      case(PWINFO_TRNORIGLOCREF    ): return "PWINFO_TRNORIGLOCREF";
      default                       : return "PWINFO_XXX";
   }
}

/*=====================================================================================*\
 Funcao     :  SalvaDadosTransacao

 Descricao  :  Responsavel por salvar os dados da transação para enviar a confirmação em caso
               de queda de energia.
 
 Entradas   :  Nao ha.

 Saidas     :  Nao ha.
 
 Retorno    :  Nao ha..
\*=====================================================================================*/
static void SalvaDadosTransacao(void) 
{
   char szAux[1024];
   FILE *fpConfirm;
   Int16 iRet, iCont = 0;


   /* Verifica se transação necessita confirmação */
   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
    if (!iRet && szAux[0] != '1') {
       /* Nao necessitando retorna */
       return;
    }

   /* Popula com dados da transação */
   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_REQNUM, szAux, sizeof(szAux));
   if (iRet != PWRET_OK && iRet != PWRET_NODATA)
      return;
   if (iRet == PWRET_OK) {
      iCont++;
      strcpy( gstConfirmData.szReqNum, szAux);
   }
   else
      memset(gstConfirmData.szReqNum, 0, sizeof(gstConfirmData.szReqNum));

   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_AUTEXTREF, szAux, sizeof(szAux));
   if (iRet != PWRET_OK && iRet != PWRET_NODATA)
      return;
   if (iRet == PWRET_OK) {
      iCont++;
      strcpy( gstConfirmData.szHostRef, szAux);
   }
   else
      strcpy( gstConfirmData.szHostRef, "");

   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_AUTLOCREF, szAux, sizeof(szAux));
   if (iRet != PWRET_OK && iRet != PWRET_NODATA)
      return;
   if (iRet == PWRET_OK) {
      iCont++;
      strcpy( gstConfirmData.szLocRef, szAux);
   }
   else
      strcpy( gstConfirmData.szLocRef, "");

   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_VIRTMERCH, szAux, sizeof(szAux));
   if (iRet != PWRET_OK && iRet != PWRET_NODATA)
      return;
   if (iRet ==PWRET_OK) {
      iCont++;
      strcpy( gstConfirmData.szVirtMerch, szAux);
   }
   else
      strcpy( gstConfirmData.szVirtMerch, "");

   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_AUTHSYST, szAux, sizeof(szAux));
   if (iRet != PWRET_OK && iRet != PWRET_NODATA)
      return;
   if (iRet == PWRET_OK) {
      iCont++;
      strcpy( gstConfirmData.szAuthSyst, szAux);
   }
   else
      strcpy( gstConfirmData.szAuthSyst, "");

   if (iCont == 0)
      return;

   fpConfirm = fopen(gszConfirmFileName, "w");
   fwrite(&gstConfirmData, sizeof(ConfirmData), 1, fpConfirm);
   fclose(fpConfirm);
}

/*=====================================================================================*\
 Funcao     :  PrintResultParams

 Descricao  :  Esta função exibe na tela todas as informações de resultado disponíveis 
               no momento em que foi chamada.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PrintResultParams()
{
   char szAux[10000];
   Int16 iRet=0, i;

   /* Primeiro salva os dados da transação */
   SalvaDadosTransacao();

   // Percorre todos os numeros inteiros, exibindo o valor do parâmetro PWINFO_XXX
   // sempre que o retorno for de dados disponível
   // Essa implementação foi feita desta forma para facilitar o exemplo, a função
   // PW_iGetResult() deve ser chamada somente para informações documentadas, as
   // chamadas adicionais causarão lentidão no sistema como um todo.
   for( i=1; i<MAXINT16; i++)
   {
      if(i==PWINFO_PPINFO || i==PWINFO_AUTHPOSQRCODE)
         continue;

      iRet = gstHwFuncs.pPW_iGetResult( i, szAux, sizeof(szAux));
      if( iRet == PWRET_OK)
      {
         InputCR(szAux);
         printf( "\n\n%s<0x%X> =\n%s", pszGetInfoDescription(i), i, szAux);

         // Caso seja um parâmetro necessário para a confirmação da transação
         // o armazena na estrutura existente para essa finalidade.
         switch(i)
         {
            case(PWINFO_REQNUM):
               strcpy( gstConfirmData.szReqNum, szAux);
               break;

            case(PWINFO_AUTEXTREF):
               strcpy( gstConfirmData.szHostRef, szAux);
               break;

            case(PWINFO_AUTLOCREF):
               strcpy( gstConfirmData.szLocRef, szAux);
               break;

            case(PWINFO_VIRTMERCH):
               strcpy( gstConfirmData.szVirtMerch, szAux);
               break;

            case(PWINFO_AUTHSYST):
               strcpy( gstConfirmData.szAuthSyst, szAux);
               break;
         }
      }
   }
   printf("\n"); // Flushing display
}

/*=====================================================================================*\
 Funcao     :  PrintReturnDescription

 Descricao  :  Esta função recebe um código PWRET_XXX e imprime na tela a sua descrição.
 
 Entradas   :  iResult :   Código de resultado da transação (PWRET_XXX). 

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PrintReturnDescription(Int16 iReturnCode, char* pszDspMsg)
{
   switch( iReturnCode)
   {
      case(PWRET_OK):
         printf("\nRetorno = PWRET_OK");
         break;

      case(PWRET_INVCALL):
         printf("\nRetorno = PWRET_INVCALL");
         break;

      case(PWRET_INVPARAM):
         printf("\nRetorno = PWRET_INVPARAM");
         break;

      case(PWRET_NODATA):
         printf("\nRetorno = PWRET_NODATA");
         break;
      
      case(PWRET_BUFOVFLW):
         printf("\nRetorno = PWRET_BUFOVFLW");
         break;

      case(PWRET_MOREDATA):
         printf("\nRetorno = PWRET_MOREDATA");
         break;

      case(PWRET_DLLNOTINIT):
         printf("\nRetorno = PWRET_DLLNOTINIT");
         break;

      case(PWRET_NOTINST):
         printf("\nRetorno = PWRET_NOTINST");
         break;

      case(PWRET_TRNNOTINIT):
         printf("\nRetorno = PWRET_TRNNOTINIT");
         break;

      case(PWRET_NOMANDATORY):
         printf("\nRetorno = PWRET_NOMANDATORY");
         break;

      case(PWRET_TIMEOUT):
         printf("\nRetorno = PWRET_TIMEOUT");
         break;

      case(PWRET_CANCEL):
         printf("\nRetorno = PWRET_CANCEL");
         break;;

      case(PWRET_FALLBACK):
         printf("\nRetorno = PWRET_FALLBACK");
         break;

      case(PWRET_DISPLAY):
         printf("\nRetorno = PWRET_DISPLAY");
         InputCR(pszDspMsg);
         printf("\n%s", pszDspMsg);
         break;

      case(PWRET_NOTHING):
         printf(".");
         break;

      case(PWRET_FROMHOST):
         printf("\nRetorno = ERRO DO HOST");
         break;

      case(PWRET_SSLCERTERR):
         printf("\nRetorno = PWRET_SSLCERTERR");
         break;

      case(PWRET_SSLNCONN):
         printf("\nRetorno = PWRET_SSLNCONN");
         break;

      default:
         printf("\nRetorno = OUTRO ERRO <%d>", iReturnCode);
         break;
   }

   // Imprime os resultados disponíveis na tela caso seja fim da transação
   if( iReturnCode!=PWRET_MOREDATA && iReturnCode!=PWRET_DISPLAY && 
       iReturnCode!=PWRET_NOTHING && iReturnCode!=PWRET_FALLBACK)
      PrintResultParams();

   fflush(stdout);
}

/*=====================================================================================*\
 Funcao     :  iExecGetData

 Descricao  :  Esta função obtém dos usuários os dados requisitado pelo Pay&Go Web.
 
 Entradas   :  vstGetData  :  Vetor com as informações dos dados a serem obtidos.
               iNumParam   :  Número de dados a serem obtidos.

 Saidas     :  nao ha.
 
 Retorno    :  Código de resultado da operação.
\*=====================================================================================*/
static Int16 iExecGetData(PW_GetData vstGetData[], Int16 iNumParam)
{
   Int16 i,j, k, iKey, iRet, iAux;
   char szAux[5081], szDspMsg[128], szMsgPinPad[34];
   static char szLastQrCode[5001];
   Uint32 ulEvent=0;

   // Caso exista uma mensagem a ser exibida antes da captura do próximo dado, a exibe
   if(vstGetData[0].szMsgPrevia != NULL && vstGetData[0].szMsgPrevia[0])
   {
      InputCR(vstGetData[0].szMsgPrevia);
      printf("\nMensagem = \n%s", vstGetData[0].szMsgPrevia);
   }
   
   // Enquanto houverem dados para capturar
   for( i=0; i < iNumParam; i++)
   {
      // Imprime na tela qual informação está sendo capturada
      printf("\nDado a capturar = %s<0x%X>", pszGetInfoDescription(vstGetData[i].wIdentificador), 
         vstGetData[i].wIdentificador);

      // Captura de acordo com o tipo de captura
      switch( vstGetData[i].bTipoDeDado)
      {
         // Menu de opções
         case(PWDAT_MENU):
            printf("\nTipo de dados = MENU");
            InputCR(vstGetData[i].szPrompt);
            printf("\n%s\n", vstGetData[i].szPrompt);

            // Caso só tenha uma opção e a opção default seja ela mesma, escolhe automaticamente
            if( vstGetData[i].bNumOpcoesMenu == 1 && vstGetData[i].bItemInicial==0)
            {
               printf("\nMENU COM 1 OPCAO... ADICIONANDO AUTOMATICAMENTE...");
               iRet = gstHwFuncs.pPW_iAddParam(vstGetData[i].wIdentificador, vstGetData[i].vszValorMenu[0]);
               if(iRet)
                  printf("\nERRO AO ADICIONAR PARAMETRO...");
               break;
            }

            // Caso o modo autoatendimento esteja ativado, faz o menu no PIN-pad
            if(gfAutoAtendimento)
            {
               if( vstGetData[i].bNumOpcoesMenu > 2)
                  printf("\nMENU NAO PODE SER FEITO NO PINPAD!!!");
               else
                  printf("\nEXECUTANDO MENU NO PINPAD");

               // Garante que as opções de menu não terão mais do que 13 caracteres
               vstGetData[i].vszTextoMenu[0][13] = '\0';
               vstGetData[i].vszTextoMenu[1][13] = '\0';

               sprintf(szMsgPinPad, "F1-%s\rF2-%s",vstGetData[i].vszTextoMenu[0], vstGetData[i].vszTextoMenu[1]);

               for(;;)
               {
                  // Exibe a mensagem no PIN-pad
                  iRet = gstHwFuncs.pPW_iPPDisplay(szMsgPinPad);
                  if(iRet)
                  {
                     printf("\nErro em PW_iPPDisplay <%d>", iRet);
                     return iRet;
                  }
                  do
                  {
                     iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
                     if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                        return iRet;
                     usleep(1000*100);
                  } while(iRet!=PWRET_OK);

                  // Aguarda a seleção da opção pelo cliente
                  ulEvent = PWPPEVTIN_KEYS;
                  iRet = gstHwFuncs.pPW_iPPWaitEvent(&ulEvent);
                  if(iRet)
                  {
                     printf("\nErro em PPWaitEvent <%d>", iRet);
                     return iRet;
                  }
                  do
                  {
                     iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
                     if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                     {
                        printf("\nErro em PW_iPPEventLoop <%d>", iRet);
                        return iRet;
                     }
                     usleep(1000*500);
                     printf(".");
                  } while(iRet!=PWRET_OK);

                  if( ulEvent==PWPPEVT_KEYF1)
                  {
                     iKey = 0x30; 
                     iAux=0;
                     break;
                  }
                  else if( ulEvent==PWPPEVT_KEYF2)
                  {
                     iKey = 0x31;
                     iAux=1;
                     break;
                  }
                  else if(ulEvent==PWPPEVT_KEYCANC)
                  {
                     iRet = gstHwFuncs.pPW_iPPDisplay("    OPERACAO        CANCELADA   ");
                     if(iRet)
                     {
                        printf("\nErro em PPDisplay <%d>", iRet);
                        return iRet;
                     }
                     do
                     {
                        iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
                        if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                           return iRet;
                        usleep(1000*100);
                     } while(iRet!=PWRET_OK);

                     return PWRET_CANCEL;
                  }
                  else
                  {
                     iRet = gstHwFuncs.pPW_iPPDisplay("   UTILIZE AS   TECLAS F1 OU F2");
                     if(iRet)
                     {
                        printf("\nErro em PPDisplay <%d>", iRet);
                        return iRet;
                     }
                     do
                     {
                        iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
                        if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                           return iRet;
                        usleep(1000*100);
                     } while(iRet!=PWRET_OK);
                     usleep(1000*1000);
                  }
               }
            }
            else
            {
               for(j=0; j < vstGetData[i].bNumOpcoesMenu; j++) 
                  if (vstGetData[i].bNumOpcoesMenu < 11)
                     printf("\n%d - %s", j, vstGetData[i].vszTextoMenu[j]);
                  else
                     printf("\n%02d - %s", j, vstGetData[i].vszTextoMenu[j]);

               printf("\n\nSELECIONE A OPCAO:");
               
               iAux = 0;
               __fpurge(stdin);

               /* Depende de quantos caracteres podem ser capturados */
               sprintf(szAux, "%d", vstGetData[i].bNumOpcoesMenu - 1);
               for (k = 0; k < strlen(szAux); k++) 
               {
                  do 
                  {
                     if(kbhit())
                     {
                        iKey = getchar ();

                        if( iKey==0x1b)
                        {
                           printf("\n\n $$$$");
                           printf("\n $$$$");
                           printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                           printf("\n $$$$");
                           printf("\n $$$$\n");

                           // Verifica se precisa resolver pendencia
                           iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                           if (!iRet && atoi(szAux) == 1) 
                           {
                              // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                              SalvaDadosTransacao();
                              iConfirmaTransacao(PWCNF_REV_ABORT);
                           }

                           return PWRET_CANCEL;
                        }

                        if( iKey<0x30 || iKey>0x39 || iKey-0x30>vstGetData[i].bNumOpcoesMenu )
                           continue;
                        else
                           break;
                     }
                  } while (TRUE);
                  if (vstGetData[i].bNumOpcoesMenu <=10){
                     iAux = (iKey - 0x30);
                     printf ("%c\n", iKey);
                  }
                  else if(k == 0) {
                     iAux = (iKey - 0x30)*10;
                     printf ("%c", iKey);
                  }
                  else {
                     iAux += (iKey - 0x30);
                     printf ("%c\n", iKey);
                  }
               }
            }

            iRet = gstHwFuncs.pPW_iAddParam(vstGetData[i].wIdentificador, vstGetData[i].vszValorMenu[iAux]);
            if(iRet)
               printf("\nERRO AO ADICIONAR PARAMETRO...");

            break;

         case (PWDAT_DSPCHECKOUT):
            InputCR(vstGetData[i].szPrompt);
            printf("\n%s\n", vstGetData[i].szPrompt);

            iRet = gstHwFuncs.pPW_iAddParam(vstGetData[i].wIdentificador, "");
            if(iRet)
               printf("\nERRO AO ADICIONAR PARAMETRO...");

            break;

         case (PWDAT_DSPQRCODE):
            InputCR(vstGetData[i].szPrompt);
            printf("\nPROMPT:%s\n", vstGetData[i].szPrompt);

            // Obtem o valor do QR code e exibe o texto (somente para efeito de teste)
            // somente se o valor tiver sido alterado
            iRet = gstHwFuncs.pPW_iGetResult( PWINFO_AUTHPOSQRCODE, szAux, sizeof(szAux));   
            if(strcmp(szLastQrCode, szAux) )
               printf("\nQR code:%s\n", szAux);

            // Armazena o ultimo QR code exibido para evitar troca-lo pelo mesmo valor
            strcpy(szLastQrCode, szAux);
            iRet = gstHwFuncs.pPW_iAddParam(vstGetData[i].wIdentificador, "");
            if(iRet)
               printf("\nERRO AO ADICIONAR PARAMETRO...");

            break;

         // Captura de dado digitado
         case(PWDAT_TYPED):
            if(gfAutoAtendimento)
            {
               gstHwFuncs.pPW_iPPAbort();
               printf("\n\nNAO E POSSIVEL CAPTURAR UM DADO DIGITADO NO AUTOATENDIMENTO\n");
               return PWRET_CANCEL;
            }

            printf("\nTipo de dados = DIGITADO");
            printf("\nTamanho minimo = %d", vstGetData[i].bTamanhoMinimo );
            printf("\nTamanho maximo = %d", vstGetData[i].bTamanhoMaximo);
            printf("\nValor atual:%s\n", vstGetData[i].szValorInicial);
            
            for(;;)
            {
               InputCR(vstGetData[i].szPrompt);
               printf("\n%s\n", vstGetData[i].szPrompt);
               if( scanf("%s", szAux) == 0)
                  szAux[0]=0;
               if( strlen(szAux) > vstGetData[i].bTamanhoMaximo)            
               {
                  printf("\nTamanho maior que o maximo permitido");
                  printf("\nTente novamente...\n");
                  continue;
               }
               else if ( strlen(szAux) < vstGetData[i].bTamanhoMinimo)
               {
                  printf("\nTamanho menor que o minimo permitido");
                  printf("\nTente novamente...\n");
                  continue;
               }
               else
                  break;
            }

            iRet = gstHwFuncs.pPW_iAddParam(vstGetData[i].wIdentificador, szAux);
            if(iRet)
               printf("\nERRO AO ADICIONAR PARAMETRO...");

            break;

         // Captura de dados do cartão
         case(PWDAT_CARDINF):
            printf("\nTipo de dados = DADOS DO CARTAO");

            if(vstGetData[i].ulTipoEntradaCartao == 1/*1=ENTRADA DIGITADA*/ )
            {
               printf(" ***SOMENTE DIGITADO***");
               InputCR(vstGetData[i].szPrompt);
               printf("\n%s\n", vstGetData[i].szPrompt);
               if( scanf("%s", szAux) == 0)
                  szAux[0]=0;

               iRet = gstHwFuncs.pPW_iAddParam(PWINFO_CARDFULLPAN, szAux);
               if(iRet)
                  printf("\nERRO AO ADICIONAR PARAMETRO...");
            }
            else
            {
               iRet = gstHwFuncs.pPW_iPPGetCard(i);
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet)
                  return iRet;
               __fpurge(stdin);
               do
               {
                  iRet = gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
                  PrintReturnDescription(iRet, szDspMsg);
                  if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                     return iRet;

                  if(kbhit())
                  {
                     iKey = getchar();
                     if(iKey==0x1b)
                     {
                        if(!gstHwFuncs.pPW_iPPAbort())
                        {
                           printf("\n\n $$$$");
                           printf("\n $$$$");
                           printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                           printf("\n $$$$");
                           printf("\n $$$$\n");

                           // Verifica se precisa resolver pendencia
                           iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                           if (!iRet && atoi(szAux) == 1) 
                           {
                              // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                              SalvaDadosTransacao();
                              iConfirmaTransacao(PWCNF_REV_ABORT);
                           }

                           return PWRET_CANCEL;
                        }
                     }
                     else if( (vstGetData[i].ulTipoEntradaCartao&1/*1=ENTRADA DIGITADA*/))
                     {
                        printf("\n\nTECLADO ACIONADO!!!\n\nDIGITE NUMERO DO CARTAO:\n");
                        gstHwFuncs.pPW_iPPAbort();
                        printf("\n");
                        if( scanf("%s", szAux) == 0)
                           szAux[0]=0;
                        iRet = gstHwFuncs.pPW_iAddParam(PWINFO_CARDFULLPAN, szAux);
                        if(iRet)
                           printf("\nERRO AO ADICIONAR PARAMETRO...");
                     break;
                     }
                  }
                  usleep(1000*500);
               } while(iRet!=PWRET_OK);
            }
            break;

         // Captura de dado digitado no PIN-pad
         case(PWDAT_PPENTRY):
            printf("\nTipo de dados = DADO DIG. NO PINPAD");
            iRet = gstHwFuncs.pPW_iPPGetData(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }

               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet; 
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         // Captura da senha criptografada
         case(PWDAT_PPENCPIN):
            printf("\nTipo de dados = SENHA");
            iRet = gstHwFuncs.pPW_iPPGetPIN(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }

               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         case (PWDAT_TSTKEY):
            printf("\nTipo de dados = TESTE DE CHAVES");
            iRet = gstHwFuncs.pPW_iPPTestKey(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }
               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);            
            break;

         // Processamento offline do cartão com chip
         case(PWDAT_CARDOFF):
            printf("\nTipo de dados = CHIP OFFLINE");
            iRet = gstHwFuncs.pPW_iPPGoOnChip(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }

               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         // Processamento online do cartão com chip
         case(PWDAT_CARDONL):
            printf("\nTipo de dados = CHIP ONLINE");
            iRet = gstHwFuncs.pPW_iPPFinishChip(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            do
            {
               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         // Confirmação de dado no PIN-pad
         case(PWDAT_PPCONF):
            printf("\nTipo de dados = PPCONFIRMDATA");
            iRet = gstHwFuncs.pPW_iPPConfirmData(i);
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }

               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         // Remoção de cartão do PIN-pad
         case(PWDAT_PPREMCRD):
            printf("\nTipo de dados = PWDAT_PPREMCRD");
            iRet = gstHwFuncs.pPW_iPPRemoveCard();
            PrintReturnDescription(iRet, szDspMsg);
            if(iRet)
               return iRet;

            __fpurge(stdin);
            do
            {
               if(kbhit() && getchar()==0x1b)
               {
                  if(!gstHwFuncs.pPW_iPPAbort())
                  {
                     printf("\n\n $$$$");
                     printf("\n $$$$");
                     printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
                     printf("\n $$$$");
                     printf("\n $$$$\n");

                     // Verifica se precisa resolver pendencia
                     iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
                     if (!iRet && atoi(szAux) == 1) 
                     {
                        // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
                        SalvaDadosTransacao();
                        iConfirmaTransacao(PWCNF_REV_ABORT);
                     }

                     return PWRET_CANCEL;
                  }
               }

               iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
               PrintReturnDescription(iRet, szDspMsg);
               if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
                  return iRet;
               usleep(1000*500);
            } while(iRet!=PWRET_OK);
            break;

         case (PWDAT_PPGENCMD):
            printf ("\nTipo de dados = PWDAT_PPGENCMD");
            
            // Executa a operação
            iRet = gstHwFuncs.pPW_iPPGenericCMD (i);
            PrintReturnDescription (iRet, szDspMsg);

            if (iRet) 
               return iRet;

            // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
            // retornos possíveis
            do {
               iRet = gstHwFuncs.pPW_iPPEventLoop (szDspMsg, sizeof (szDspMsg));
               PrintReturnDescription (iRet, szDspMsg);
               if (iRet != PWRET_OK && iRet != PWRET_DISPLAY && iRet != PWRET_NOTHING) {
                  return iRet;
               }

               usleep (1000 * 500);
            } while (iRet != PWRET_OK);

            break;

         case (PWDAT_PPDATAPOSCNF):
            printf("\nTipo de dados = PWDAT_PPDATAPOSCNF");
            
            // Executa a operação
            iRet = gstHwFuncs.pPW_iPPPositiveConfirmation (i);
            PrintReturnDescription (iRet, szDspMsg);

            if (iRet) 
               return iRet;

            // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
            // retornos possíveis
            printf ("\nIniciando EventLoop...");
            do {
               iRet = gstHwFuncs.pPW_iPPEventLoop (szDspMsg, sizeof (szDspMsg));
               PrintReturnDescription (iRet, szDspMsg);
               if (iRet != PWRET_OK && iRet != PWRET_DISPLAY && iRet != PWRET_NOTHING) {
                  return iRet;
               }

               usleep (1000 * 500);
            } while (iRet != PWRET_OK);
            break;

         case (PWDAT_USERAUTH):
            printf("\nTipo de dados = PWDAT_USERAUTH");

            if(vstGetData[0].wIdentificador == PWINFO_AUTHMNGTUSER) 
            {
               printf("\n\nDIGITE A SENHA DO LOJISTA (4 DIGITOS):\n");
               if( scanf("%s", szAux) == 0)
                  szAux[0]=0;
               if(!strcmp( szAux, gszUserPassword))
                  iRet = gstHwFuncs.pPW_iAddParam(PWINFO_AUTHMNGTUSER, szAux);
               else 
               {
                  printf("\n\nSENHA INCORRETA!");
                  return PWRET_CANCEL;
               }
            }
            else 
            {
               printf("\n\nDIGITE A SENHA TECNICA (6 DIGITOS):\n");
               if( scanf("%s", szAux) == 0)
                  szAux[0]=0;
               if(!strcmp( szAux, gszTechPassword))
                  iRet = gstHwFuncs.pPW_iAddParam(PWINFO_AUTHTECHUSER, szAux);
               else  
               {
                  printf("\n\nSENHA INCORRETA!");
                  return PWRET_CANCEL;
               }
            }

            if(iRet)
               printf("\nERRO AO ADICIONAR PARAMETRO...");

            break;

         // Tipo de captura desconhecido
         default:
            printf("\nCAPTURA COM TIPO DE DADOS DESCONHECIDO<%d>", vstGetData[i].bTipoDeDado);
      }
   }
   return PWRET_OK;
}

/*===========================================================================*\
 Função   : iGetStringEx 

 Descrição: Função complexa para capturar um dado na tela de console.

 Entradas : pszMsg = Mensagem (prompt) a ser apresentada para captura do dado.
            iMaxChar = Quantidade máxima de caracteres.
            wFlags = Combinação das constantes GETS_xxx.
            pszData = Dado default a ser sugerido.

 Saídas   : pszData = Dado capturado.

 Retorno  : Quantidade de caracteres capturados, ou -1 se captura cancelada.
\*===========================================================================*/
static int iGetStringEx (const char *pszMsg, int iMaxChar, Word wFlags, char *pszData)
{
   int iIdx, iKey, i, iMsgLen, iOffset = 0;

   iMsgLen = (int) strlen (pszMsg);

   /* Assume a posição inicial do cursor (iIdx) */
   if (wFlags & GETS_NOSZ) 
   {
      /* Quando não tem final NUL... */
      if (wFlags & GETS_NODEFAULT)
         iIdx = 0;
      else 
         iIdx = iMaxChar;
   } 
   else 
   {
      /* Quando tem final NUL... */
      if (--iMaxChar < 0) 
         return 0;

      if (wFlags & GETS_NODEFAULT)
         iIdx = 0;
      else 
         iIdx = (int) strlen (pszData);

      if (iIdx > iMaxChar) 
         return -1;
   }

   /* Coloca a mensagem e o espaço para digitação (com o valor default) */
   if (wFlags & GETS_FIXEDLEN) 
   {
      printf ("\n%s", pszMsg);
      iOffset = 6 + iMsgLen;
      if (iMaxChar < CONSOLE_WIDTH - iOffset - 1) 
      {
         for (i = 0; i < iMaxChar; i++) 
            printf (" ");
         printf ("]");
         printf ("\r%s", pszMsg);
      }
   } 
   else 
   {
      if (wFlags & GETS_ASCCR) 
      {
         printf ("\n%s (ASC, #=CR) : ", pszMsg);
         iOffset = 18 + iMsgLen;
      } 
      else 
      {
         printf ("\n%s", pszMsg);
         iOffset = 6 + iMsgLen;
      }
   }

   for (i = 0; i < iIdx; i++, iOffset++) 
      printf ("%c", pszData[i]);

   /* Loop de entrada de dados */
   __fpurge(stdin);
   for (;;) 
   {
      if (!(wFlags & GETS_NOSZ))
         pszData[iIdx] = 0;

      iKey = getchar ();

      if (iKey >= ' ' && iIdx < iMaxChar) 
      {
         printf ("%c", iKey);
         if (iKey == '#' && (wFlags & GETS_ASCCR)) 
            iKey = '\r';
         pszData[iIdx++] = (char) iKey;
         iOffset++;
         continue;
      }

      /* Pressionado ENTER... */
      if (iKey == 13 || iKey == 10) 
      {
         // Verica se há restrição de # par de dígitos...
         if (!(wFlags & GETS_EVEN) || !(iIdx % 2)) 
         {
            // Se não for tamanho fixo sempre acata...
            if (!(wFlags & GETS_FIXEDLEN)) 
               break;

            // Se atingiu o tamanho máximo sempre acata...
            if (iIdx >= iMaxChar) 
               break;
              
            // Se campo estiver vazio e isto á permitido, acata...
            if (!iIdx && (wFlags & GETS_ACCEPTBLK)) 
               break;
         }
      }

      if (iKey == 27)
         return -1;

      if (iKey == 8 && iIdx > 0 && iOffset < CONSOLE_WIDTH) 
      {
         printf ("\b \b");
         iIdx--;
         iOffset--;
      }    
   }
   
   return iIdx;
}

/*=====================================================================================*\
 Funcao     :  AddMandatoryParams

 Descricao  :  Esta função adiciona os parâmetros obrigatórios de toda mensagem para o
               Pay&Go Web.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void AddMandatoryParams(Uint32 ulAutCap)
{
   char szAutCap[9];

   // Adiciona os parâmetros obrigatórios
   gstHwFuncs.pPW_iAddParam(PWINFO_AUTDEV, "SETIS AUTOMACAO E SISTEMA LTDA");
   gstHwFuncs.pPW_iAddParam(PWINFO_AUTVER, PGWEBLIBTEST_VERSION);
   gstHwFuncs.pPW_iAddParam(PWINFO_AUTNAME, "PGWEBLIBTEST");
   gstHwFuncs.pPW_iAddParam(PWINFO_AUTHTECHUSER, "PGWEBLIBTEST");
   sprintf(szAutCap, "%lu", ulAutCap);
   gstHwFuncs.pPW_iAddParam(PWINFO_AUTCAP, szAutCap);

   // A linha abaixo indica a preferência da automação por exibir o QR Code em seu próprio
   // display, caso a transação seja efetuada com uma carteira digital, se esse valor não for
   // informado, o QR Code será exibido no PIN-pad para automações convencionais e retornado 
   // para exibição no checkout, caso o ponto de captura esteja configurado como autoatendimento
   // IMPORTANTE: A biblioteca só irá retornar o QR Code para a automação, ao invés de exibí-lo
   // no PIN-pad, caso a automação informe que tem a capacidade de tratá-lo, passando o valor
   // AUTCAP_DSPQRCODE somados às capacidades em PWINFO_AUTCAP
   
   //gstHwFuncs.pPW_iAddParam(PWINFO_DSPQRPREF, "2");
}

/*=====================================================================================*\
 Funcao     :  Init

 Descricao  :  Esta função captura os dados necesários e executa PW_iInit.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void Init()
{
   char szWorkingDir[256];
   Int16 iRet=0;
   FILE *fpConfirmation = NULL;

   printf("\nDigite a pasta de trabalho: ");
   if( scanf(" %[^\n]", szWorkingDir) == 0)
      szWorkingDir[0]=0;

   iRet = gstHwFuncs.pPW_iInit(szWorkingDir);
   if (!iRet) {
      /* Verifica se existe arquivo de confirmação */
      fpConfirmation = fopen(gszConfirmFileName, "r");
      if (fpConfirmation != NULL) 
      {
         memset(&gstConfirmData, 0, sizeof(ConfirmData));
         while (fread(&gstConfirmData, sizeof(ConfirmData), 1, fpConfirmation));
         iConfirmaTransacao(PWCNF_CNF_AUTO);
         if (gstConfirmData.szAuthSyst[0] || gstConfirmData.szHostRef[0] || gstConfirmData.szLocRef[0] ||
            gstConfirmData.szReqNum[0] || gstConfirmData.szVirtMerch[0]) 
         {
               //UTL_AddTimeLogLine("Confirmação realizada");
               printf("\nTransacao confirmada automaticamente devido a queda de energia!");
         }
         fclose(fpConfirmation);
         remove(gszConfirmFileName);
      }
   }

   PrintReturnDescription(iRet, NULL);
}

/*=====================================================================================*\
 Funcao     :  NewTransac

 Descricao  :  Esta função captura os dados necesários e executa PW_iNewTransac.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void NewTransac()
{
   Int16 iOper = 0, iRet = 0;
  
   // Exibe na tela as opções de operação existentes e captura a escolhida
   printf("\n0x00 - PWOPER_NULL       - Testa comunicacao");
   printf("\n0x01 - PWOPER_INSTALL    - Instalacao");
   printf("\n0x02	- PWOPER_PARAMUPD	  - Atualizacao de parametros");
   printf("\n0x10 - PWOPER_REPRINT    - Reimpressao");
   printf("\n0x11 - PWOPER_RPTTRUNC   - Relatorio");
   printf("\n0x20 - PWOPER_ADMIN      - Operacao administrativa");
   printf("\n0x21 - PWOPER_SALE       - Venda");
   printf("\n0x22 - PWOPER_SALEVOID   - Cancelamento de venda");
   printf("\n0x23 - PWOPER_PREPAID    - Aquisicao de creditos pre-pagos");
   printf("\n0x24 - PWOPER_CHECKINQ   - Consulta a validade de um cheque papel");
   printf("\n0x25 - PWOPER_RETBALINQ  - Consulta o saldo/limite do Estabelecimento");
   printf("\n0x26 - PWOPER_CRDBALINQ  - Consulta o saldo do cartao do Cliente");
   printf("\n0x27 - PWOPER_INITIALIZ  - Inicializacao/abertura");
   printf("\n0x28 - PWOPER_SETTLEMNT  - Fechamento/finalizacao");
   printf("\n0x29 - PWOPER_PREAUTH    - Pre-autorizacao");
   printf("\n0x2A - PWOPER_PREAUTVOID - Cancelamento de pre-autorizacao");
   printf("\n0x2B - PWOPER_CASHWDRWL  - Saque");
   printf("\n0x2C - PWOPER_LOCALMAINT - Baixa tecnica");
   printf("\n0x2D - PWOPER_FINANCINQ  - Consulta as taxas de financiamento");
   printf("\n0x2E - PWOPER_ADDRVERIF  - Verifica junto ao Provedor o endereco do Cliente");
   printf("\n0x2F - PWOPER_SALEPRE    - Efetiva uma pre-autorizacao (PWOPER_PREAUTH)");
   printf("\n0x30 - PWOPER_LOYCREDIT  - Registra o acumulo de pontos pelo Cliente");
   printf("\n0x31 - PWOPER_LOYCREDVOID- Cancela uma transacao PWOPER_LOYCREDIT");
   printf("\n0x32 - PWOPER_LOYDEBIT   - Registra o resgate de pontos/premio pelo Cliente");
   printf("\n0x33 - PWOPER_LOYDEBVOID - Cancela uma transacao PWOPER_LOYDEBIT");
   printf("\n0x39 - PWOPER_VOID       - Menu de cancelamentos, se so 1 seleciona automaticamente");
   printf("\n0xFC - PWOPER_VERSION    - Versao");
   printf("\n0xFD - PWOPER_CONFIG     - Configuracao");
   printf("\n0xFE - PWOPER_MAINTENANCE- Manutencao");
   printf("\n SELECIONE A OPCAO:");
   if( scanf("%hx", &iOper) == 0)
      iOper=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iNewTransac(iOper);
   PrintReturnDescription(iRet, NULL);

   __fpurge(stdin);
   printf ("\n <tecle algo>\n\n");
   getchar();
}

/*=====================================================================================*\
 Funcao     :  AddParam

 Descricao  :  Esta função captura os dados necesários e executa PW_iAddParam.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void AddParam()
{
   Int16 iParam = 0, iRet = 0;
   char szValue[129];

   // Exibe na tela as opções de informação existentes e captura a escolhida
   printf("\nEscolha o codigo do parametro a ser adicionado: ");
   printf("\n0x11 - PWINFO_POSID		");	
   printf("\n0x15 - PWINFO_AUTNAME		");	
   printf("\n0x16 - PWINFO_AUTVER		");	
   printf("\n0x17 - PWINFO_AUTDEV		");	
   printf("\n0x1B - PWINFO_DESTTCPIP	");	
   printf("\n0x1C - PWINFO_MERCHCNPJCPF");	
   printf("\n0x24 - PWINFO_AUTCAP		");	
   printf("\n0x25 - PWINFO_TOTAMNT		");	
   printf("\n0x26 - PWINFO_CURRENCY	");	
   printf("\n0x27 - PWINFO_CURREXP		");	
   printf("\n0x28 - PWINFO_FISCALREF	");	
   printf("\n0x29 - PWINFO_CARDTYPE	");	
   printf("\n0x2A - PWINFO_PRODUCTNAME	");	
   printf("\n0x31 - PWINFO_DATETIME	");	
   printf("\n0x32 - PWINFO_REQNUM		");	
   printf("\n0x35 - PWINFO_AUTHSYST	");	
   printf("\n0x36 - PWINFO_VIRTMERCH	");	
   printf("\n0x38 - PWINFO_AUTMERCHID	");	
   printf("\n0x3A - PWINFO_PHONEFULLNO	");	
   printf("\n0x3B - PWINFO_FINTYPE		");	
   printf("\n0x3C - PWINFO_INSTALLMENTS");	
   printf("\n0x3D - PWINFO_INSTALLMDATE");	
   printf("\n0x3E - PWINFO_PRODUCTID	");	
   printf("\n0x42 - PWINFO_RESULTMSG	");	
   printf("\n0x43 - PWINFO_CNFREQ		");	
   printf("\n0x44 - PWINFO_AUTLOCREF	");	
   printf("\n0x45 - PWINFO_AUTEXTREF	");	
   printf("\n0x46 - PWINFO_AUTHCODE	");	
   printf("\n0x47 - PWINFO_AUTRESPCODE	");	
   printf("\n0x48 - PWINFO_AUTDATETIME	");	
   printf("\n0x49 - PWINFO_DISCOUNTAMT	");	
   printf("\n0x4A - PWINFO_CASHBACKAMT	");	
   printf("\n0x4B - PWINFO_CARDNAME	");	
   printf("\n0x4C - PWINFO_ONOFF		");	
   printf("\n0x4D - PWINFO_BOARDINGTAX	");	
   printf("\n0x4E - PWINFO_TIPAMOUNT	");	
   printf("\n0x4F - PWINFO_INSTALLM1AMT");	
   printf("\n0x50 - PWINFO_INSTALLMAMNT");	
   printf("\n0x52 - PWINFO_RCPTFULL	");	
   printf("\n0x53 - PWINFO_RCPTMERCH	");	
   printf("\n0x54 - PWINFO_RCPTCHOLDER	");	
   printf("\n0x55 - PWINFO_RCPTCHSHORT	");	
   printf("\n0x57 - PWINFO_TRNORIGDATE	");	
   printf("\n0x58 - PWINFO_TRNORIGNSU	");	
   printf("\n0x60 - PWINFO_TRNORIGAMNT	");	
   printf("\n0x62 - PWINFO_TRNORIGAUTH	");	
   printf("\n0x72 - PWINFO_TRNORIGREQNUM");	
   printf("\n0x73 - PWINFO_TRNORIGTIME	");
   printf("\n0x78 - PWINFO_TRNORIGLOCREF ");
   printf("\n0xC1 - PWINFO_CARDFULLPAN	");	
   printf("\n0xC2 - PWINFO_CARDEXPDATE	");	
   printf("\n0xC8 - PWINFO_CARDPARCPAN	");	
   printf("\n0xE9 - PWINFO_BARCODENTMODE");
   printf("\n0xEA - PWINFO_BARCODE		");	
   printf("\n0xF0 - PWINFO_MERCHADDDATA1");	
   printf("\n0xF1 - PWINFO_MERCHADDDATA2");	
   printf("\n0xF2 - PWINFO_MERCHADDDATA3");
   printf("\n0xF3 - PWINFO_MERCHADDDATA4");
   printf("\n0xF5 - PWINFO_AUTHMNGTUSER");      
   printf("\n0xF6 - PWINFO_AUTHTECHUSER");
   printf("\n0x1F21 - PWINFO_PAYMNTTYPE");
   printf("\n0x1F81 - MUXTAG_WALLETUSERIDTYPE");
   printf("\n0x7F01 - PWINFO_USINGPINPAD");		
   printf("\n0x7F02 - PWINFO_PPCOMMPORT");		
   printf("\n0x7F04 - PWINFO_IDLEPROCTIME"); 
   printf("\n0xBF90 - MUXTAG_UNIQUEID");
   printf("\nCodigo = ");
   if( scanf("%hx", &iParam) == 0)
      iParam=0;

   printf("\nDigite o valor do parametro: ");
   if( scanf(" %[^\n]",szValue) == 0)
      szValue[0]=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iAddParam(iParam, szValue);
   PrintReturnDescription(iRet, NULL);

   __fpurge(stdin);
   printf ("\n <tecle algo>\n\n");
   getchar();
}

/*=====================================================================================*\
 Funcao     :  ExecTransac

 Descricao  :  Esta função executa PW_iExecTransac, caso falte algum dado para executar
               a transação, faz a captura dos dados faltantes do usuário e executa 
               PW_iExecTransac novamente, até que seja possível aprovar/negar a transação.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void ExecTransac()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet=0;
   
   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      // Tenta executar a transação
      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");

      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes = %d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING)
         continue;
      break;     
   }

   __fpurge(stdin);
   printf ("\n <tecle algo>\n\n");
   getchar();
}

/*=====================================================================================*\
 Funcao     :  GetResult

 Descricao  :  Esta função obtém um dado da transação através de PW_iGetResult.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void GetResult()
{
   Int16 iParam = 0, iRet = 0;
   char szAux[10000];

   // Exibe na tela as opções de informação existentes e captura a escolhida
   printf("\nEscolha o codigo do parametro a ser obtido: ");
   printf("\n0x11 - PWINFO_POSID		");	
   printf("\n0x15 - PWINFO_AUTNAME		");	
   printf("\n0x16 - PWINFO_AUTVER		");	
   printf("\n0x17 - PWINFO_AUTDEV		");	
   printf("\n0x1B - PWINFO_DESTTCPIP	");	
   printf("\n0x1C - PWINFO_MERCHCNPJCPF");	
   printf("\n0x24 - PWINFO_AUTCAP		");	
   printf("\n0x25 - PWINFO_TOTAMNT		");	
   printf("\n0x26 - PWINFO_CURRENCY	");	
   printf("\n0x27 - PWINFO_CURREXP		");	
   printf("\n0x28 - PWINFO_FISCALREF	");	
   printf("\n0x29 - PWINFO_CARDTYPE	");	
   printf("\n0x2A - PWINFO_PRODUCTNAME	");	
   printf("\n0x31 - PWINFO_DATETIME	");	
   printf("\n0x32 - PWINFO_REQNUM		");	
   printf("\n0x35 - PWINFO_AUTHSYST	");	
   printf("\n0x36 - PWINFO_VIRTMERCH	");	
   printf("\n0x38 - PWINFO_AUTMERCHID	");	
   printf("\n0x3A - PWINFO_PHONEFULLNO	");	
   printf("\n0x3B - PWINFO_FINTYPE		");	
   printf("\n0x3C - PWINFO_INSTALLMENTS");	
   printf("\n0x3D - PWINFO_INSTALLMDATE");	
   printf("\n0x3E - PWINFO_PRODUCTID	");	
   printf("\n0x42 - PWINFO_RESULTMSG	");	
   printf("\n0x43 - PWINFO_CNFREQ		");	
   printf("\n0x44 - PWINFO_AUTLOCREF	");	
   printf("\n0x45 - PWINFO_AUTEXTREF	");	
   printf("\n0x46 - PWINFO_AUTHCODE	");	
   printf("\n0x47 - PWINFO_AUTRESPCODE	");	
   printf("\n0x48 - PWINFO_AUTDATETIME	");	
   printf("\n0x49 - PWINFO_DISCOUNTAMT	");	
   printf("\n0x4A - PWINFO_CASHBACKAMT	");	
   printf("\n0x4B - PWINFO_CARDNAME	");	
   printf("\n0x4C - PWINFO_ONOFF		");	
   printf("\n0x4D - PWINFO_BOARDINGTAX	");	
   printf("\n0x4E - PWINFO_TIPAMOUNT	");	
   printf("\n0x4F - PWINFO_INSTALLM1AMT");	
   printf("\n0x50 - PWINFO_INSTALLMAMNT");	
   printf("\n0x52 - PWINFO_RCPTFULL	");	
   printf("\n0x53 - PWINFO_RCPTMERCH	");	
   printf("\n0x54 - PWINFO_RCPTCHOLDER	");	
   printf("\n0x55 - PWINFO_RCPTCHSHORT	");	
   printf("\n0x57 - PWINFO_TRNORIGDATE	");	
   printf("\n0x58 - PWINFO_TRNORIGNSU	");	
   printf("\n0x60 - PWINFO_TRNORIGAMNT	");	
   printf("\n0x62 - PWINFO_TRNORIGAUTH	");	
   printf("\n0x72 - PWINFO_TRNORIGREQNUM");	
   printf("\n0x73 - PWINFO_TRNORIGTIME	");
   printf("\n0x78 - PWINFO_TRNORIGLOCREF ");
   printf("\n0xC1 - PWINFO_CARDFULLPAN	");	
   printf("\n0xC2 - PWINFO_CARDEXPDATE	");
   printf("\n0xC4 - PWINFO_CARDNAMESTD ");
   printf("\n0xC8 - PWINFO_CARDPARCPAN	");	
   printf("\n0xE9 - PWINFO_BARCODENTMODE");
   printf("\n0xEA - PWINFO_BARCODE		");	
   printf("\n0xF0 - PWINFO_MERCHADDDATA1");	
   printf("\n0xF1 - PWINFO_MERCHADDDATA2");	
   printf("\n0xF2 - PWINFO_MERCHADDDATA3");
   printf("\n0xF3 - PWINFO_MERCHADDDATA4");	
   printf("\n0x1F21 - PWINFO_PAYMNTTYPE");
   printf("\n0x1F81 - MUXTAG_WALLETUSERIDTYPE");
   printf("\n0x7F01 - PWINFO_USINGPINPAD");		
   printf("\n0x7F02 - PWINFO_PPCOMMPORT");		
   printf("\n0x7F04 - PWINFO_IDLEPROCTIME");
   printf("\n0xBF90 - MUXTAG_UNIQUEID");
   printf("\nCodigo = ");
   if( scanf("%hx", &iParam) == 0)
      iParam=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iGetResult( iParam, szAux, sizeof(szAux));
   PrintReturnDescription(iRet, NULL);

   __fpurge(stdin);
   printf ("\n <tecle algo>\n\n");
   getchar();
}

/*=====================================================================================*\
 Funcao     :  Confirmation

 Descricao  :  Esta função captura os dados necesários e executa PW_iConfirmation.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void Confirmation()
{
   Int16 iOpc=0, iRet=0;
   Uint32 ulStatus=0;
   char  szAux[129];
   
   printf("\n1 - PWCNF_CNF_AUT");	    
   printf("\n2 - PWCNF_CNF_MANU_AUT");	 
   printf("\n3 - PWCNF_REV_MANU_AUT");	 
   printf("\n4 - PWCNF_REV_PRN_AU");
   printf("\n5 - PWCNF_REV_DISP_AUT");
   printf("\n6 - PWCNF_REV_COMM_AUT");
   printf("\n7 - PWCNF_REV_ABORT");	 
   printf("\n8 - PWCNF_REV_OTHER_AUT");
   printf("\n9 - PWCNF_REV_PWR_AUT");
   printf("\n10 -PWCNF_REV_FISC_AUT");

   printf ("\n SELECIONE A OPCAO:");

   if( scanf("%hd", &iOpc) == 0)
      iOpc=0;

   switch(iOpc)
   {
      case(1): 
         ulStatus = PWCNF_CNF_AUTO;
         break;

      case(2): 
         ulStatus = PWCNF_CNF_MANU_AUT;
         break;
      
      case(3): 
         ulStatus = PWCNF_REV_MANU_AUT;
         break;
      
      case(4): 
         ulStatus = PWCNF_REV_DISP_AUT;
         break;
      
      case(5): 
         ulStatus = PWCNF_REV_DISP_AUT;
         break;

      case(6): 
         ulStatus = PWCNF_REV_COMM_AUT;
         break;

      case(7): 
         ulStatus = PWCNF_REV_ABORT;
         break;
         
      case(8): 
         ulStatus = PWCNF_REV_OTHER_AUT;
         break;
         
      case(9): 
         ulStatus = PWCNF_REV_PWR_AUT;
         break;
      
      case(10): 
         ulStatus = PWCNF_REV_FISC_AUT;
         break;
   }

   // Pergunta ao usuário se ele deseja utilizar as informações armazenadas da ultima 
   // transação ou se deseja inserir manualmente as informações de uma transação qualquer
   // a ser confirmada
   printf ("\n (0) UTILIZAR INFORMACOES DA ULTIMA TRANSACAO PARA A CONFIRMACAO:");
   printf ("\n (1) CAPTURAR AS INFORMACOES PARA A CONFIRMACAO:");

   printf ("\n SELECIONE A OPCAO:");
   if( scanf("%hd", &iOpc) == 0)
      iOpc=0;

   // Caso seja necessário capturar as informações da transação do usuário
   if(iOpc)
   {
      printf("\nDigite o valor de PWINFO_REQNUM (0(zero) para parametro ausente): ");
      if( scanf(" %[^\n]",szAux) == 0)
         szAux[0]=0;
      if( atoi(szAux) == 0)
         memset( gstConfirmData.szReqNum, 0, sizeof(gstConfirmData.szReqNum));
      else
         strcpy(gstConfirmData.szReqNum, szAux);

      printf("\nDigite o valor de PWINFO_AUTLOCREF (0(zero) para parametro ausente): ");
      if( scanf(" %[^\n]",szAux) == 0)
         szAux[0]=0;
      if( atoi(szAux) == 0)
         memset( gstConfirmData.szLocRef, 0, sizeof(gstConfirmData.szLocRef));
      else
         strcpy(gstConfirmData.szLocRef, szAux);

      printf("\nDigite o valor de PWINFO_AUTEXTREF (0(zero) para parametro ausente): ");
      if( scanf(" %[^\n]",szAux) == 0)
         szAux[0]=0;
      if( atoi(szAux) == 0)
         memset( gstConfirmData.szHostRef, 0, sizeof(gstConfirmData.szHostRef));
      else
         strcpy(gstConfirmData.szHostRef, szAux);

      printf("\nDigite o valor de PWINFO_VIRTMERCH: ");
      if( scanf(" %[^\n]",szAux) == 0)
         szAux[0]=0;
      if( atoi(szAux) == 0)
         memset( gstConfirmData.szVirtMerch, 0, sizeof(gstConfirmData.szVirtMerch));
      else
         strcpy(gstConfirmData.szVirtMerch, szAux);

      printf("\nDigite o valor de PWINFO_AUTHSYST (0(zero) para parametro ausente): ");
      if( scanf(" %[^\n]",szAux) == 0)
         szAux[0]=0;
      if( !strcmp(szAux, "0"))
         memset( gstConfirmData.szAuthSyst, 0, sizeof(gstConfirmData.szAuthSyst));
      else
         strcpy(gstConfirmData.szAuthSyst, szAux);
   }

   printf("\n\nPROCESSANDO...\n");
   iRet = gstHwFuncs.pPW_iConfirmation(ulStatus, gstConfirmData.szReqNum, 
      gstConfirmData.szLocRef, gstConfirmData.szHostRef, gstConfirmData.szVirtMerch, 
      gstConfirmData.szAuthSyst);
   /* Caso o arquivo de confirmação tenha sido usado, apaga-o */
   if (!iOpc)
      remove(gszConfirmFileName);

   PrintReturnDescription(iRet, NULL);

   // Zera a estrutura que armazena informações para confirmação da transação
   memset( &gstConfirmData, 0, sizeof(gstConfirmData));  
}

/*=====================================================================================*\
 Funcao     :  IdleProc

 Descricao  :  Esta função executa PW_iIdleProc.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void IdleProc()
{
   Int16 iRet=0;
   
   iRet = gstHwFuncs.pPW_iIdleProc();
   PrintReturnDescription(iRet, NULL);
}

/*=====================================================================================*\
 Funcao     :  PPGetCard

 Descricao  :  Esta função executa PW_iPPGetCard.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGetCard()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPGetCard(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {
      printf("\nIniciando EventLoop...");
      do
      {
         iRet = gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPGetPIN

 Descricao  :  Esta função executa PW_iPPGetPIN.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGetPIN()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPGetPIN(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet = gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      }while(iRet!=PWRET_OK); 
   }
}

/*=====================================================================================*\
 Funcao     :  PPGetData

 Descricao  :  Esta função executa PW_iPPGetData.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGetData()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPGetData(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPGoOnChip

 Descricao  :  Esta função executa PW_iPPGoOnChip.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGoOnChip()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPGoOnChip(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPFinishChip

 Descricao  :  Esta função executa PW_iPPFinishChip.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPFinishChip()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPFinishChip(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPConfirmData

 Descricao  :  Esta função executa PW_iPPConfirmData.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPConfirmData()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint16 uiIndex=0;

   // Obtém o índice da captura, parâmetro obrigatório
   printf("\nIndice da captura = ");
   if( scanf("%hu", &uiIndex) == 0)
      uiIndex=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPConfirmData(uiIndex);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPRemoveCard

 Descricao  :  Esta função executa PW_iPPRemoveCard.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPRemoveCard()
{
   Int16 iRet;
   char szDspMsg[128];

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPRemoveCard();
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPDisplay

 Descricao  :  Esta função executa PW_iPPDisplay.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPDisplay()
{
   Int16 iRet;
   char szDspMsg[128];
   char szAux[40];

   // Obtém a mensagem para exibição
   printf("\nDigite a mensagem a ser exibida no PIN-pad:");
   if( scanf(" %[^\n]",szAux) == 0)
      szAux[0]=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPDisplay(szAux);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*100);
      } while(iRet!=PWRET_OK);
   }
}

/*=====================================================================================*\
 Funcao     :  PPWaitEvent

 Descricao  :  Esta função executa PW_iPPWaitEvent.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPWaitEvent()
{
   Int16 iRet;
   char szDspMsg[128];
   Uint32 ulEvent=0;

   // Obtém quais eventos devem ser monitorados
   printf("\n1 - TECLAS");
   printf("\n2 - CARTAO MAGNETICO");
   printf("\n4 - INSERCAO DE CHIP");
   printf("\n8 - APROXIMACAO SEM CONTATO");
   printf("\n16 - REMOCAO DE CHIP");

   printf ("\n INDIQUE A SOMA DOS EVENTOS A SEREM MONITORADOS:");
   if( scanf("%lu", &ulEvent) == 0)
      ulEvent=0;

   // Executa a operação
   iRet = gstHwFuncs.pPW_iPPWaitEvent(&ulEvent);
   PrintReturnDescription(iRet, szDspMsg);
   if(iRet)
      return;

   // Caso a operação tenha sido executada com sucesso, inicia o loop e trata os
   // retornos possíveis
   if(iRet == PWRET_OK)
   {

      printf("\nIniciando EventLoop...");
      __fpurge(stdin);
      do
      {
         if(kbhit() && getchar ()==0x1b)
         {
            if(!gstHwFuncs.pPW_iPPAbort())
            {
               printf("\n\n $$$$");
               printf("\n $$$$");
               printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
               printf("\n $$$$");
               printf("\n $$$$\n");
               return;
            }
         }

         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         PrintReturnDescription(iRet, szDspMsg);
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);

      switch(ulEvent)
      {
         case(PWPPEVT_MAGSTRIPE):
            printf("\n\nEVENTO = PWPPEVT_MAGSTRIPE");
            break;

         case(PWPPEVT_ICC):
            printf("\n\nEVENTO = PWPPEVT_ICC");
            break;

         case(PWPPEVT_ICCOUT):
            printf("\n\nEVENTO = PWPPEVT_ICCOUT");
            break;

         case(PWPPEVT_CTLS):
            printf("\n\nEVENTO = PWPPEVT_CTLS");
            break;

         case(PWPPEVT_KEYCONF):
            printf("\n\nEVENTO = PWPPEVT_KEYCONF");
            break;

         case(PWPPEVT_KEYBACKSP):
            printf("\n\nEVENTO = PWPPEVT_KEYBACKSP");
            break;

         case(PWPPEVT_KEYCANC):
            printf("\n\nEVENTO = PWPPEVT_KEYCANC");
            break;

         case(PWPPEVT_KEYF1):
            printf("\n\nEVENTO = PWPPEVT_KEYF1");
            break;

         case(PWPPEVT_KEYF2):
            printf("\n\nEVENTO = PWPPEVT_KEYF2");
            break;

         case(PWPPEVT_KEYF3):
            printf("\n\nEVENTO = PWPPEVT_KEYF3");
            break;

         case(PWPPEVT_KEYF4):
            printf("\n\nEVENTO = PWPPEVT_KEYF4");
            break;
      }
   }
}
  
/*=====================================================================================*\
 Funcao     :  GetOperations

 Descricao  :  Esta função executa PW_iGetOperations.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void GetOperations()
{
   Int16 iRet=0, iNumOperations, i;
   PW_Operations vstOperations[100];
   Byte          bOper=0;

   iNumOperations = 100;

   // Obtém o tio de operação a ser obtida
   printf("\nTipo de operacao (1)ADM (2) VENDA (3)AMBAS = ");
   if(scanf("%hhu", &bOper)==0)
      bOper=0;

   iRet = gstHwFuncs.pPW_iGetOperations( bOper, vstOperations, &iNumOperations);
   PrintReturnDescription(iRet, NULL);

   if(iRet == PWRET_OK)
   {
      printf( "\n\n");
      for(i=0; i<iNumOperations; i++)
      {
         printf( "\nTYPE = %d OPER = %20s COD = %s", vstOperations[i].bOperType, vstOperations[i].szText, vstOperations[i].szValue);
      }
   }
}  

/*=====================================================================================*\
 Funcao     :  GetOperationsEx

 Descricao  :  Esta função executa PW_iGetOperationsEx.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void GetOperationsEx()
{
   Int16 iRet=0, iNumOperations, i;
   PW_OperationsEx vstOperations[100];
   Byte          bOper=0;

   iNumOperations = 100;

   // Obtém o tio de operação a ser obtida
   printf("\nTipo de operacao (1)ADM (2) VENDA (3)AMBAS = ");
   if(scanf("%hhu", &bOper)==0)
      bOper=0;

   iRet = gstHwFuncs.pPW_iGetOperationsEx( bOper, vstOperations, sizeof(PW_OperationsEx), &iNumOperations);
   PrintReturnDescription(iRet, NULL);

   if(iRet == PWRET_OK)
   {
      printf( "\n\n");
      for(i=0; i<iNumOperations; i++)
      {
         printf( "\nTYPE = %d AuthSyst %20s OPER = %20s COD = %s fAuthPreferential %d", vstOperations[i].bOperType, vstOperations[i].szAuthSyst,
            vstOperations[i].szOperName, vstOperations[i].szValue, vstOperations[i].fAuthPreferential);
      }
   }
} 

/*=====================================================================================*\
 Funcao     :  TransactionInquiry

 Descricao  :  Esta função executa PW_iTransactionInquiry.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TransactionInquiry()
{
   Int16 iRet=0;
   char szXMLRequest[256000], szXMLResponse[256000], szAux[128];

   // Obtém a mensagem para exibição
   printf("\nDigite a data da consulta no formato DD/MM/AAA:");
   if( scanf(" %[^\n]",szAux) == 0)
      szAux[0]=0;

   sprintf(szXMLRequest, "<RequestTransactionHistory xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" version=\"1.0.0.0\"><authentication><login>integracao</login><password>123456</password></authentication><QueryFilter><TransactionAuthorizer></TransactionAuthorizer><TransactionStartDate>%s 00:00:00</TransactionStartDate><TransactionEndDate>%s 23:59:59</TransactionEndDate><ArrayOfTransactionStatus><TransactionStatus>289</TransactionStatus></ArrayOfTransactionStatus></QueryFilter><Error><Number/><Description/></Error></RequestTransactionHistory>", szAux, szAux);

   iRet = gstHwFuncs.pPW_iTransactionInquiry( szXMLRequest, szXMLResponse, sizeof(szXMLResponse));
   PrintReturnDescription(iRet, NULL);

   if(iRet == PWRET_OK)
   {
      printf( "\n\n%s", szXMLResponse);
   }
}

/*=====================================================================================*\
 Funcao     :  PPGetUserData

 Descricao  :  Esta função executa PW_iPPGetUserData.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGetUserData()
{
   Uint16   uiMessageId=0, uiMinLen=0, uiMaxLen=0;
   Int16    iToutSec=0, iRet=0;
   char     szDadoDigitado[33];

   // Obtém o índice da mensagem a ser exibida ao usuário
   printf("\nIndice da mensagem a ser exibida:");
   if( scanf("%hu", &uiMessageId) == 0)
      uiMessageId=0;

   // Obtém o tamanho minimo do dado a ser capturado
   printf("\nDigite o tamanho minimo do dado a ser capturado:");
   if( scanf("%hu", &uiMinLen) == 0)
      uiMinLen=0;

   // Obtém o tamanho maximo do dado a ser capturado
   printf("\nDigite o tamanho maximo do dado a ser capturado:");
   if( scanf("%hu", &uiMaxLen) == 0)
      uiMaxLen=0;

   // Obtém o tempo limite para a captura
   printf("\nDigite o tempo limite para a captura:");
   if( scanf("%hd", &iToutSec) == 0)
      iToutSec=0;

   iRet = gstHwFuncs.pPW_iPPGetUserData(uiMessageId, (Byte)uiMinLen, (Byte)uiMaxLen, iToutSec, szDadoDigitado);
   PrintReturnDescription(iRet, NULL);

   if(iRet == PWRET_OK)
   {
      printf( "\n\n***** DADO DIGITADO: %s \n\n", szDadoDigitado);
   }
}

/*=====================================================================================*\
 Funcao     :  PPGetPINBlock

 Descricao  :  Esta função executa PW_iPPGetPINBlock.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void PPGetPINBlock()
{
   Uint16      uiMinLen=0, uiMaxLen=0;
   Int16       iToutSec=0, iRet=0;
   char        szPINBlock[17];
   // Dois valores diferentes irão gerar PIN blocks diferentes, mesmo que a senha digitada seja a mesma
   const char  szWorkingKey[] = "12345678901234567890123456789012";
   // Sempre com 32 caracteres
   const char  szPrompt[] = "TESTE DE CAPTURA\rDE PIN BLOCK:   ";

   // Obtém o tamanho minimo do dado a ser capturado
   printf("\nDigite o tamanho minimo do dado a ser capturado:");
   if( scanf("%hu", &uiMinLen) == 0)
      uiMinLen=0;

   // Obtém o tamanho maximo do dado a ser capturado
   printf("\nDigite o tamanho maximo do dado a ser capturado:");
   if(scanf("%hu", &uiMaxLen) == 0)
      uiMaxLen=0;

   // Obtém o tempo limite para a captura
   printf("\nDigite o tempo limite para a captura:");
   if( scanf("%hd", &iToutSec) == 0)
      iToutSec=0;

   iRet = gstHwFuncs.pPW_iPPGetPINBlock(12, szWorkingKey, (Byte)uiMinLen, (Byte)uiMaxLen, iToutSec, szPrompt, szPINBlock);

   PrintReturnDescription(iRet, NULL);

   if(iRet == PWRET_OK)
   {
      printf( "\n\n***** PIN BLOCK: %s \n\n", szPINBlock);
   }
}

/*=====================================================================================*\
 Funcao     :  TesteInstalacao

 Descricao  :  Esta função inicia uma transação de instalação através de PW_iNewTransac,
               adiciona os parâmetros obrigatórios através de PW_iAddParam e em seguida 
               se comporta de forma idêntica a ExecTransac.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteInstalacao()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet=0;

   // Inicializa a transação de instalação
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_INSTALL);
   if( iRet)
      printf("\nErro PW_iNewTransac <%d>", iRet);

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP);
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");

   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING)
         continue;
      break;
   }
}

/*=====================================================================================*\
 Funcao     :  TesteRecarga

 Descricao  :  Esta função inicia uma transação de recarga através de PW_iNewTransac,
               adiciona os parâmetros obrigatórios através de PW_iAddParam e em seguida 
               se comporta de forma idêntica a ExecTransac.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteRecarga()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet=0;

   // Inicializa a transação de instalação
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_PREPAID);
   if( iRet)
      printf("\nErro PW_iNewTransac <%d>", iRet);

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP);
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");
   
   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING)
         continue;
      break;
   }
}

/*=====================================================================================*\
 Funcao     :  TesteVenda

 Descricao  :  Esta função inicia uma transação de venda através de PW_iNewTransac,
               adiciona os parâmetros obrigatórios através de PW_iAddParam e em seguida 
               se comporta de forma idêntica a ExecTransac.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteVenda()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet=0;
   char szAux[13], cKey;

   /// Inicializa a transação de venda
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_SALE);
   if( iRet) 
   {
      printf("\nErro PW_iNewTransac <%d>", iRet);
      PrintReturnDescription(iRet, NULL);
      return;
   };

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP);
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");

   //Adicionado os parâmetros de venda e código da moeda antes da captura dos outros dados
   iGetStringEx("Digite o valor da transacao: ", sizeof(szAux), GETS_NODEFAULT, szAux);
  
   gstHwFuncs.pPW_iAddParam(PWINFO_TOTAMNT, szAux);
   gstHwFuncs.pPW_iAddParam(PWINFO_CURRENCY, "986");
   gstHwFuncs.pPW_iAddParam(PWINFO_CURREXP, "2");    


   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   __fpurge(stdin);
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING) 
      {
         /* Condição que checa a tecla ESC pressionada em casos onde retornou PWRET_NOTHING*/
         if(kbhit() && getchar ()==0x1b)
         {   
            // Permite que a biblioteca feche o PIN-pad caso a operação seja cancelada na 
            // automação e exista um QR code sendo exibido no PIN-pad
            gstHwFuncs.pPW_iPPAbort();

            printf("\n\n $$$$");
            printf("\n $$$$");
            printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
            printf("\n $$$$");
            printf("\n $$$$\n");

            // Verifica se precisa resolver pendencia
            iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
            if (!iRet && atoi(szAux) == 1) 
            {
               // Pega os identificadores da transação e desfaz ela por cancelamento de usuário
               SalvaDadosTransacao();
               iConfirmaTransacao(PWCNF_REV_ABORT);
            }

            return;
         }
         continue;
      }
      break;  
   }

   /* Verifica se transação necessita confirmação */
   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
   if (!iRet && szAux[0] != '1') 
   {
      /* Não necessitando retorna */
      return;
   }

   printf("\nDeseja confirmar a transacao ? ");
   printf("\n0 - NAO ");
   printf("\n1 - SIM ");
   __fpurge(stdin);
   for (;;) 
   {
      if(kbhit())
      {
         cKey = getchar();
         if (cKey == '0') 
         {
            iConfirmaTransacao(PWCNF_REV_PRN_AUT);
            return;
         }
         else if (cKey == '1') 
         {
            iConfirmaTransacao(PWCNF_CNF_AUTO);
            return;
         }
         else 
         {
            if( cKey==0x1b)
            {
               printf("\n\n $$$$");
               printf("\n $$$$");
               printf("\n $$$$ CONFIRMACAO CANCELADA PELO USUARIO");
               printf("\n $$$$");
               printf("\n $$$$\n");
               return ;
            }
         }
      }
   }
}

/*=====================================================================================*\
 Funcao     :  TesteAdmin

 Descricao  :  Esta função inicia uma transação administrativa genérica através de 
               PW_iNewTransac, adiciona os parâmetros obrigatórios através de PW_iAddParam 
               e em seguida se comporta de forma idêntica a ExecTransac.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteAdmin()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet=0;
   char szAux[13], cKey;

   /// Inicializa a transação de instalação
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_ADMIN);
   if( iRet)
      printf("\nErro PW_iNewTransac <%d>", iRet);

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP);
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");

   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   __fpurge(stdin);
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING) 
      {
         /* Condição que checa a tecla ESC pressionada em casos onde retornou PWRET_NOTHING*/
         if(kbhit() && getchar()==0x1b)
         {   
            printf("\n\n $$$$");
            printf("\n $$$$");
            printf("\n $$$$ OPERACAO CANCELADA PELO USUARIO");
            printf("\n $$$$");
            printf("\n $$$$\n");
            return;
         }
         continue;
      }
      break;
   }

   /* Verifica se transação necessita confirmação */
   iRet = gstHwFuncs.pPW_iGetResult(PWINFO_CNFREQ, szAux, sizeof(szAux));
   if (!iRet && szAux[0] != '1') 
   {
      /* Não necessitando retorna */
      return;
   }

   printf("\nDeseja confirmar a transacao ? ");
   printf("\n0 - NAO ");
   printf("\n1 - SIM ");
   __fpurge(stdin);
   for (;;) 
   {
      if(kbhit())
      {
         cKey = getchar();
         if (cKey == '0') 
         {
            iConfirmaTransacao(PWCNF_REV_PRN_AUT);
            return;
         }
         else if (cKey == '1') 
         {
            iConfirmaTransacao(PWCNF_CNF_AUTO);
            return;
         }
         else 
         {
            if( cKey==0x1b)
            {
               printf("\n\n $$$$");
               printf("\n $$$$");
               printf("\n $$$$ CONFIRMACAO CANCELADA PELO USUARIO");
               printf("\n $$$$");
               printf("\n $$$$\n");
               return ;
            }
         }
      }
   }
}

/*=====================================================================================*\
 Funcao     :  TesteAutoatendimento

 Descricao  :  Esta função aguarda o uso de um cartão como "gatilho" para início da 
               transação, após isso inicia uma transação de venda através de PW_iNewTransac,
               adiciona os parâmetros obrigatórios através de PW_iAddParam e em seguida 
               se comporta de forma idêntica a ExecTransac, confirmando a transação no 
               final do fluxo.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteAutoatendimento()
{
   PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet;
   Uint32 ulEvent=0;
   char szDspMsg[128];

   // Aguarda a inserção ou passagem do cartão pelo usuário para iniciar a transação
   for(;;)
   {
      // Exibe a mensagem no PIN-pad
      printf("\nAGUARDANDO CARTAO PARA INICIAR OPERACAO!!!");
      iRet = gstHwFuncs.pPW_iPPDisplay(" INSIRA OU PASSE    O CARTAO    ");
      if(iRet)
      {
         printf("\nErro em PW_iPPDisplay <%d>", iRet);
         return;
      }
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*100);
      } while(iRet!=PWRET_OK);

      // Aguarda o cartão do cliente
      iRet = gstHwFuncs.pPW_iPPWaitEvent(&ulEvent);
      if(iRet)
      {
         printf("\nErro em PPWaitEvent <%d>", iRet);
         return;
      }
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
         {
            printf("\nErro em PW_iPPEventLoop <%d>", iRet);
            return;
         }
         usleep(1000*500);
         printf(".");
      } while(iRet!=PWRET_OK);

      if( ulEvent==PWPPEVT_ICC || ulEvent==PWPPEVT_MAGSTRIPE || ulEvent== PWPPEVT_CTLS)                  
         break;
   }

   // Exibe mensagem processando no PIN-pad
   iRet = gstHwFuncs.pPW_iPPDisplay(" PROCESSANDO...                 ");
   if(iRet)
   {
      printf("\nErro em PPDisplay <%d>", iRet);
      return;
   }
   do
   {
      iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
      if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
         return;
      usleep(1000*100);
   } while(iRet!=PWRET_OK);

   /// Inicializa a transação de venda
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_SALE);
   if( iRet)
      printf("\nErro PW_iNewTransacr <%d>", iRet);

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP);
   gstHwFuncs.pPW_iAddParam(PWINFO_TOTAMNT, "100");
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");
   
   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING)
         continue;
      break;
   }

   // Caso a transação tenha ocorrido com sucesso, confirma
   if( iRet == PWRET_OK)
   {
      printf("\n\nCONFIRMANDO TRANSACAO...\n");
      iRet = gstHwFuncs.pPW_iConfirmation(PWCNF_CNF_AUTO, gstConfirmData.szReqNum, 
      gstConfirmData.szLocRef, gstConfirmData.szHostRef, gstConfirmData.szVirtMerch, 
      gstConfirmData.szAuthSyst);
      if( iRet)
         printf("\nERRO AO CONFIRMAR TRANSAçãO");
   }

}

/*=====================================================================================*\
 Funcao     :  TesteRemoveCard

 Descricao  :  Esta função inicia uma transação de venda através de PW_iNewTransac,
               adiciona os parâmetros obrigatórios através de PW_iAddParam (incluindo a 
               capacidade AUTCAP_REMOCAOCARTAO da automação tratar por conta própria a 
               remoção do cartão) e em seguida se comporta de forma idêntica a ExecTransac, 
               confirmando a transação no final do fluxo e fazendo aremoção do cartão por
               conta própria, visto que a passagem do parâmetro AUTCAP_REMOCAOCARTAO para
               a biblioteca fará com que a mesma finalize a transação mantendo o cartão
               inserido no PIN-pad.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void TesteRemoveCard()
{
  PW_GetData vstParam[10];
   Int16   iNumParam = 10, iRet;
   Uint32 ulEvent=0;
   char szDspMsg[128], szAux[128];

   /// Inicializa a transação de venda
   iRet = gstHwFuncs.pPW_iNewTransac(PWOPER_SALE);
   if( iRet)
      printf("\nErro PW_iNewTransacr <%d>", iRet);

   // Adiciona os parâmetros obrigatórios
   AddMandatoryParams(PGWLT_AUTCAP|AUTCAP_REMOCAOCARTAO);
   gstHwFuncs.pPW_iAddParam(PWINFO_CTLSCAPTURE, "1");
   
   // Loop até que ocorra algum erro ou a transação seja aprovada, capturando dados
   // do usuário caso seja necessário
   for(;;)
   {
      // Coloca o valor 10 (tamanho da estrutura de entrada) no parâmetro iNumParam
      iNumParam = 10;

      if(iRet != PWRET_NOTHING)
         printf("\n\nPROCESSANDO...\n");
      iRet = gstHwFuncs.pPW_iExecTransac(vstParam, &iNumParam);
      PrintReturnDescription(iRet, NULL);
      if(iRet == PWRET_MOREDATA)
      {
          printf("\nNumero de parametros ausentes:%d", iNumParam);
         
          // Tenta capturar os dados faltantes, caso ocorra algum erro retorna
         if (iExecGetData(vstParam, iNumParam))
            return;
         continue;
      }
      else if(iRet == PWRET_NOTHING)
         continue;
      break;
   }

   // Caso a transação tenha ocorrido com sucesso, confirma
   if( iRet == PWRET_OK)
   {
      printf("\n\nCONFIRMANDO TRANSACAO...\n");
      iRet = gstHwFuncs.pPW_iConfirmation(PWCNF_CNF_AUTO, gstConfirmData.szReqNum, 
      gstConfirmData.szLocRef, gstConfirmData.szHostRef, gstConfirmData.szVirtMerch, 
      gstConfirmData.szAuthSyst);
      if( iRet)
         printf("\nERRO AO CONFIRMAR TRANSAçãO");
   }

   // Caso tenha sido feita uma transação com cartão com chip
   // Como foi passado o parâmetro AUTCAP_REMOCAOCARTAO nas capacidades da automação, o cartão permanecerá
   // inserido no PIN-pad, sendo de responsabilidade da automação fazer essa remoção
   
   iRet = gstHwFuncs.pPW_iGetResult( PWINFO_CARDENTMODE, szAux, sizeof(szAux));
   if(atoi(szAux)==CARTAO_CHIP)
   {
      // Exibe a mensagem no PIN-pad e na console para remoção do cartão
      printf("\nRETIRE O CARTAO !!!");
      iRet = gstHwFuncs.pPW_iPPDisplay("     RETIRE         O CARTAO    ");
      if(iRet)
      {
         printf("\nErro em PW_iPPDisplay <%d>", iRet);
         return;
      }
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
            return;
         usleep(1000*500);
      } while(iRet!=PWRET_OK);

      // Aguarda a ausência do cartão
      ulEvent = PWPPEVTIN_ICCOUT;
      iRet = gstHwFuncs.pPW_iPPWaitEvent(&ulEvent);
      if(iRet)
      {
         printf("\nErro em PPWaitEvent <%d>", iRet);
         return;
      }
      do
      {
         iRet =gstHwFuncs.pPW_iPPEventLoop(szDspMsg, sizeof(szDspMsg));
         if(iRet!=PWRET_OK && iRet!=PWRET_DISPLAY && iRet!=PWRET_NOTHING)
         {
            printf("\nErro em PW_iPPEventLoop <%d>", iRet);
            return;
         }
         usleep(1000*500);
         printf(".");
      } while(iRet!=PWRET_OK);

      if( ulEvent!=PWPPEVT_ICCOUT)                  
      {
         printf("\nEvento nao esperado retornado");
         return;
      }
   }
   printf("\nTRANSACAO FINALIZADA !!!");
}

/*=====================================================================================*\
 Funcao     :  iTestLoop

 Descricao  :  Esta função oferece ao usuário um menu com todas as operações possíveis e 
               captura a operação desejada pelo usuário, executando-a. Este loop só para
               se o usuário selecionar a opção SAIR.
 
 Entradas   :  nao ha.

 Saidas     :  nao ha.
 
 Retorno    :  nao ha.
\*=====================================================================================*/
static void iTestLoop (void)
{
   Int16   iKey;

   for(;;)
   {
      // Exibe as opções
      printf ("\n===================");
      printf ("\nFUNCOES");
      printf ("\n===================");
      printf("\n0 - SAIR");
      printf("\n1 - PW_iInit");
      printf("\n2 - PW_iNewTransac");
      printf("\n3 - PW_iAddParam");
      printf("\n4 - PW_iExecTransac");
      printf("\n5 - PW_iGetResult");
      printf("\n6 - PW_iConfirmation");
      printf("\n7 - PW_iIdleProc");
      printf("\n8 - PW_iPPGetCard");  
      printf("\n9 - PW_iPPGetPIN");   
      printf("\na - PW_iPPGetData"); 
      printf("\nb - PW_iPPGoOnChip");
      printf("\nc - PW_iPPFinishChip");
      printf("\nd - PW_iPPConfirmData");
      printf("\ne - PW_iPPRemoveCard");
      printf("\nf - PW_iPPDisplay");
      printf("\ng - PW_iPPWaitEvent");
      printf("\nh - PW_iGetOperations");
      printf("\nm - PW_iGetOperationsEx");
      printf("\ni - PW_iTransactionInquiry");
      printf("\nj - PW_iPPGetUserData");
      printf("\nk - PW_iPPGetPINBlock");
      printf("\nl - Teste RemoveCard");
      printf("\nv - Teste autoatendimento");
      printf("\nx - Teste instalacao");
      printf("\ny - Teste recarga");
      printf("\nz - Teste venda");
      printf("\nw - Teste admin");

      

      printf ("\n\nESCOLHA A FUNCAO: "); 

      __fpurge(stdin);
      iKey=0;
      // Verifica se foi selecionada uma opção válida
      do {
         if(kbhit())
            iKey = getchar ();
      } while (iKey != '0' && iKey != '1' && iKey != '2' && iKey != '3' && iKey != '4' && iKey != '5' &&
               iKey != '6' && iKey != '7' && iKey != '8' && iKey != '9' && iKey != 'a' && iKey != 'b' &&
               iKey != 'c' && iKey != 'd' && iKey != 'e' && iKey != 'f' && iKey != 'g' && iKey != 'h' && 
               iKey != 'x' && iKey != 'y' && iKey != 'z' && iKey != 'w' && iKey != 'v' && iKey != 'i' && 
               iKey != 'j' && iKey != 'k' && iKey != 'l' && iKey != 'm');
      printf ("%c\n", iKey);
   
      // Executa a operação selecionada pelo usuário
      switch( iKey)
      {
         case('1'):    
            Init();
            break;
         case('2'):    
            NewTransac();
            break;
         case('3'):
            AddParam();
            break;
         case('4'):
            ExecTransac();
            break;
         case('5'):    
            GetResult();
            break;
         case('6'):    
            Confirmation();
            break;
         case('7'):    
            IdleProc();
            break;
         case('8'):    
            PPGetCard();
            break;
         case('9'):    
            PPGetPIN();
            break;
         case('a'):    
            PPGetData();
            break;
         case('b'):    
            PPGoOnChip();
            break;
         case('c'):    
            PPFinishChip();
            break;
         case('d'):  
            PPConfirmData();     
            break;
         case('e'):  
            PPRemoveCard();     
            break;
         case('f'):  
            PPDisplay();     
            break;
         case('g'):  
            PPWaitEvent();     
            break;
         case('h'):
            GetOperations();
            break;
         case('m'):
            GetOperationsEx();
            break;

         case('x'):
            TesteInstalacao();        
            break;
         case('y'):
            TesteRecarga();        
            break;
         case('z'):
            TesteVenda();
            break;
         case('w'):
            TesteAdmin();
            break;


         case('v'):
            gfAutoAtendimento = TRUE;

            TesteAutoatendimento();

            gfAutoAtendimento = FALSE;
            break;

         case('i'):
            TransactionInquiry();
            break;

         case('j'):
            PPGetUserData();
            break;

         case('k'):
            PPGetPINBlock();
            break;

         case('l'):
            TesteRemoveCard();
            break;

         default:       
            return;
            break;        
      }
   }
}

/******************************************/
/* FUNÇÃO DE ENTRADA DO APLICATIVO        */
/******************************************/
int main(int argc, char* argv[])
{
   printf ("\n\n");
   printf ("\n========================================");
   printf ("\nPGWebLibTest v%s", PGWEBLIBTEST_VERSION);
   printf ("\nPrograma de testes da biblioteca");
   printf ("\nPay&Go WEB - DLL de integracao");
   printf ("\n========================================");
   printf ("\n");

   // Tenta carregar a DLL que deve estar na mesma pasta do executável deste aplicativo
   ghHwLib = dlopen("./PGWebLib.so", RTLD_LAZY);

   // Caso não consiga carregar a DLL retorna erro
   if( ghHwLib == NULL)
   {
      printf ("\n\nNao foi possivel carregar PGWebLib.so\n%s\n\n", dlerror());
   }
   else
   {
      // Carrega todas as funções exportadas pela DLL para uso no aplicativo
      gstHwFuncs.pPW_iInit          = dlsym  (ghHwLib, "PW_iInit");
      if( gstHwFuncs.pPW_iInit == NULL)
         printf ("\n\nNao foi possivel carregar PW_iInit\n\n");
      gstHwFuncs.pPW_iNewTransac    = dlsym  (ghHwLib, "PW_iNewTransac");
      if( gstHwFuncs.pPW_iNewTransac == NULL)
         printf ("\n\nNao foi possivel carregar PW_iNewTransac\n\n");
      gstHwFuncs.pPW_iAddParam      = dlsym  (ghHwLib, "PW_iAddParam");
      if( gstHwFuncs.pPW_iAddParam == NULL)
         printf ("\n\nNao foi possivel carregar PW_iAddParam\n\n");
      gstHwFuncs.pPW_iExecTransac   = dlsym  (ghHwLib, "PW_iExecTransac");
      if( gstHwFuncs.pPW_iExecTransac == NULL)
         printf ("\n\nNao foi possivel carregar PW_iExecTransac\n\n");
      gstHwFuncs.pPW_iGetResult     = dlsym  (ghHwLib, "PW_iGetResult");
      if( gstHwFuncs.pPW_iGetResult == NULL)
         printf ("\n\nNao foi possivel carregar PW_iGetResult\n\n");
      gstHwFuncs.pPW_iConfirmation  = dlsym  (ghHwLib, "PW_iConfirmation");
      if( gstHwFuncs.pPW_iConfirmation == NULL)
         printf ("\n\nNao foi possivel carregar PW_iConfirmation\n\n");
      gstHwFuncs.pPW_iIdleProc      = dlsym  (ghHwLib, "PW_iIdleProc");
      if( gstHwFuncs.pPW_iIdleProc == NULL)
         printf ("\n\nNao foi possivel carregar PW_iIdleProc\n\n");
      gstHwFuncs.pPW_iPPAbort       = dlsym  (ghHwLib, "PW_iPPAbort");
      if( gstHwFuncs.pPW_iPPAbort == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPAbort\n\n");
      gstHwFuncs.pPW_iPPEventLoop   = dlsym  (ghHwLib, "PW_iPPEventLoop");
      if( gstHwFuncs.pPW_iPPEventLoop == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPEventLoop\n\n");
      gstHwFuncs.pPW_iPPGetCard     = dlsym  (ghHwLib, "PW_iPPGetCard");
      if( gstHwFuncs.pPW_iPPGetCard == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGetCard\n\n");
      gstHwFuncs.pPW_iPPGetPIN      = dlsym  (ghHwLib, "PW_iPPGetPIN");
      if( gstHwFuncs.pPW_iPPGetPIN == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGetPIN\n\n");
      gstHwFuncs.pPW_iPPGetData     = dlsym  (ghHwLib, "PW_iPPGetData");
      if( gstHwFuncs.pPW_iPPGetData == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGetData\n\n");
      gstHwFuncs.pPW_iPPGoOnChip    = dlsym  (ghHwLib, "PW_iPPGoOnChip");
      if( gstHwFuncs.pPW_iPPGoOnChip == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGoOnChip\n\n");
      gstHwFuncs.pPW_iPPFinishChip  = dlsym  (ghHwLib, "PW_iPPFinishChip");
      if( gstHwFuncs.pPW_iPPFinishChip == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPFinishChip\n\n");
      gstHwFuncs.pPW_iPPConfirmData = dlsym  (ghHwLib, "PW_iPPConfirmData");
      if( gstHwFuncs.pPW_iPPConfirmData == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPConfirmData\n\n");
      gstHwFuncs.pPW_iPPRemoveCard  = dlsym  (ghHwLib, "PW_iPPRemoveCard");
      if( gstHwFuncs.pPW_iPPRemoveCard == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPRemoveCard\n\n");
      gstHwFuncs.pPW_iPPDisplay     = dlsym  (ghHwLib, "PW_iPPDisplay");
      if( gstHwFuncs.pPW_iPPDisplay == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPDisplay\n\n");
      gstHwFuncs.pPW_iPPWaitEvent   = dlsym  (ghHwLib, "PW_iPPWaitEvent");
      if( gstHwFuncs.pPW_iPPWaitEvent == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPWaitEvent\n\n");
      gstHwFuncs.pPW_iPPGenericCMD  = dlsym  (ghHwLib, "PW_iPPGenericCMD");
      if (gstHwFuncs.pPW_iPPGenericCMD == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGenericCMD\n\n");
      gstHwFuncs.pPW_iTransactionInquiry   = dlsym  (ghHwLib, "PW_iTransactionInquiry");
      if( gstHwFuncs.pPW_iTransactionInquiry == NULL)
         printf ("\n\nNao foi possivel carregar PW_iTransactionInquiry\n\n");
      gstHwFuncs.pPW_iPPGetUserData  = dlsym  (ghHwLib, "PW_iPPGetUserData");
      if (gstHwFuncs.pPW_iPPGetUserData == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGetUserData\n\n");
      gstHwFuncs.pPW_iPPGetPINBlock  = dlsym  (ghHwLib, "PW_iPPGetPINBlock");
      if (gstHwFuncs.pPW_iPPGetPINBlock == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPGetPINBlock\n\n");

      gstHwFuncs.pPW_iGetOperations  = dlsym  (ghHwLib, "PW_iGetOperations");
      if (gstHwFuncs.pPW_iGetOperations == NULL)
         printf ("\n\nNao foi possivel carregar PW_iGetOperations\n\n");
      gstHwFuncs.pPW_iGetOperationsEx  = dlsym  (ghHwLib, "PW_iGetOperationsEx");
      if (gstHwFuncs.pPW_iGetOperationsEx == NULL)
         printf ("\n\nNao foi possivel carregar PW_iGetOperationsEx\n\n");
      gstHwFuncs.pPW_iPPTestKey  = dlsym  (ghHwLib, "PW_iPPTestKey");
      if (gstHwFuncs.pPW_iPPTestKey == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPTestKey\n\n");
      gstHwFuncs.pPW_iPPPositiveConfirmation  = dlsym  (ghHwLib, "PW_iPPPositiveConfirmation");
      if (gstHwFuncs.pPW_iPPPositiveConfirmation == NULL)
         printf ("\n\nNao foi possivel carregar PW_iPPPositiveConfirmation\n\n");

      // Inicia o Loop oferecendo as opçães possíveis e aguardando o usuário selecionar
      iTestLoop ();

      // Libera a memória alocada pela biblioteca
#ifndef _DEBUG_
      dlclose (ghHwLib);
#endif
   }

   __fpurge(stdin);
   printf ("\n\nPROGRAMA FINALIZADO\n   <tecle algo>\n\n");
   getchar();

   return 0;
}