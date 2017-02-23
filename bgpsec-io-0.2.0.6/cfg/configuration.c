/**
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to title 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 *
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.
 *
 * We would appreciate acknowledgment if the software is used.
 *
 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 *
 *
 * This software might use libraries that are under GNU public license or
 * other licenses. Please refer to the licenses of all libraries required
 * by this software.
 *
 * This header file contains data structures needed for the application.
 *
 * @version 0.2.0.6
 * 
 * ChangeLog:
 * -----------------------------------------------------------------------------
 *  0.2.0.6 - 2017/02/15 - oborchert
 *            * Added switch to force sending extended messages regardless if
 *              capability is negotiated. This is a TEST setting only.
 *          - 2017/02/13 - oborchert
 *            * Renamed define from ..._EXTMSG_SIZE to EXT_MSG_CAP
 *            * Removed invalid DEPRECATION message
 *            * BZ1111: Added liberal policy to extended message capability 
 *              processing
 *  0.2.0.5 - 2017/01/31 - oborchert
 *            * Added missing configuration for extended message size capability
 *          - 2017/01/03 - oborchert
 *            * Added parameter P_CFG_SIGMODE
 *          - 2016/11/15 - oborchert
 *            * Added parameter P_CFG_ONLY_EXTENDED_LENGTH
 *          - 2016/10/21 - oborchert
 *            * Fixed issue with 32/64 bit libconfig integer type BZ1033.
 *  0.2.0.3 - 2016/06/28 - oborchert
 *            * Added missing description of parameter -U to help output.
 *  0.2.0.2 - 2016/06/27 - oborchert
 *            * Added warning message in case useMPNLRI is set to false.
 *            * Added --version / -v to print version number
 *            * Also added version to help screen
 *          - 2016/06/24 - oborchert
 *            * Fixed BUG 923 - Added detection for invalid scripted updates
 *  0.2.0.0 - 2016/06/08 - oborchert
 *            * Fixed memory leak
 *          - 2016/05/13 - oborchert
 *            * Added maximum update processing BZ:961
 *          - 2016/05/11 - oborchert
 *            * Re-arranged help output.
 *          - 2016/05/06 - oborchert
 *            * Added processing of configuration for SRxCryptoAPI cofiguration.
 *            * Replaced sprintf with snprintf for filename specifications.
 *            * Fixed a bug if the prefix misses the prefix length (BZ: 948)
 *  0.1.1.0 - 2016/04/27 - oborchert
 *            * Removed debug printout.
 *          - 2016/04/19 - oborchert
 *            * Added write boundary for filenames to not produce a segmentation
 *              fault.
 *          - 2016/04/15 - oborchert
 *            * Fixed prefix generation in createUpdate. Generate prefix in 
 *              big-endian (network) format.
 *            * Set prefix to SAFI_UNICAST in update generation.
 *          - 2016/03/26 - oborchert
 *            * Added initialization of algoParam in session init. 
 *          - 2016/03/21 - oborchert
 *            * fixed BZ892
 *            * Added more specific print instructions for BGP traffic.
 *          - 2016/03/17 - oborchert
 *            * Modified _createUpdate to allow an empty path. This will allow
 *              bgpsec-io to generate a one hop path with a prefix originated
 *              by bgpsec-io itself.
 *  0.1.0.0 - 2015/08/26 - oborchert
 *            * Created File.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <libconfig.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "antd-util/prefix.h"
#include "antd-util/printer.h"
#include "antd-util/stack.h"
#include "cfg/configuration.h"
#include "bgp/BGPSession.h"
#include "bgp/BGPHeader.h"
#include "bgp/printer/BGPPrinterUtil.h"

/**
 * This message fills the error message with the given string.
 * 
 * @param param
 */
static void _setErrMsg(PrgParams* param, char* str)
{
  int buffLen = PARAM_ERRBUF_SIZE;
  memset (&param->errMsgBuff, '\0', buffLen);

  int size = strlen(str) < buffLen ? strlen(str) : (buffLen - 1);
  // Only copy the amount of text into the error message as space is available
  memcpy(&param->errMsgBuff, str, size);
}


/**
 * Print the program Syntax.
 */
void printSyntax()
{
  printf ("Syntax: %s [parameters]\n", PRG_NAME);
  printf (" Parameters: \n");
  // Help
  printf ("  -%c, -%c, -%c, %s\n", P_C_HELP, P_C_HELP_1, P_C_HELP_2, P_HELP);
  printf ("          This screen!\n");

  printf ("  -%c, %s\n", P_C_VERSION, P_VERSION);
  printf ("          Display the version number.\n");
  
  printf ("  -%c <config>, %s <config>\n", P_C_CONFIG, P_CONFIG);
  printf ("          config: the configuration file.\n");

  printf ("  -%c <config>, %s <config>\n", P_C_CAPI_CFG, P_CAPI_CFG);
  printf ("          config: an alternative SRxCryptoAPI configuration file.\n");
  
  // Update: <prefix,path>
  printf ("  -%c <prefix, path>, %s <prefix, path>\n", P_C_UPD_PARAM, P_UPD_PARAM);
  printf ("          prefix: prefix to be announced.\n");
  printf ("          path: the list of AS numbers (right most is origin).\n");
  
  // SKI_FILE
  printf ("  -%c <filename>, %s <filename>\n", P_C_SKI_FILE, P_SKI_FILE);
  printf ("          Name of the SKI file generated by qsrx-publish\n");
  //SKI_LOC
  printf ("  -%c <direcotry>, %s <directory>\n", P_C_SKI_LOC, P_SKI_LOC);
  printf ("          Specify the location where the keys and certificates"
                     " are located.\n");
  // type
  printf ("  -%c <type>, %s <type>\n", P_C_TYPE, P_TYPE);
  printf ("          Enable the operational mode:\n");
  printf ("          type BGP: run BGP player\n");
  printf ("          type CAPI: run as SRxCryptoAPI tester.\n");
  printf ("          type GEN: Generate the binary data.\n");
  
  // ASN  
  printf ("  -%c <asn>, %s <asn>\n", P_C_MY_ASN, P_MY_ASN);
  printf ("          Specify the own AS number.\n");
  // BGP identifier
  printf ("  -%c <IPv4>, %s <IPv4>\n", P_C_BGP_IDENT, P_BGP_IDENT);
  printf ("          The BGP identifier of the BGP daemon.\n");
  // Hold Timer
  printf ("  -%c <time>, %s <time>\n", P_C_HOLD_TIME, P_HOLD_TIME);
  printf ("          The hold timer in seconds (0 or >=3).\n");
  // Peer ASN
  printf ("  -%c <asn>, %s <asn>\n", P_C_PEER_AS, P_PEER_AS);
  printf ("          The peer as number.\n");
  // Peer IP
  printf ("  -%c <IPv4>, %s <IPv4>\n", P_C_PEER_IP, P_PEER_IP);
  printf ("          The IP address of the peer.\n");
  // Peer Port
  printf ("  -%c <port>, %s <port>\n", P_C_PEER_PORT, P_PEER_PORT);
  printf ("          The port number of the peer.\n");
  
  printf ("  -%c, %s\n", P_C_NO_MPNLRI, P_NO_MPNLRI);
  printf ("          DEPRECATED.\n");
  printf ("          Disable MPNLRI encoding for IPv4 addresses.\n");
  printf ("          If disabled prefixes are encoded as NLRI only.\n");

  printf ("  -%c, %s\n", P_C_NO_EXT_MSG_CAP, P_NO_EXT_MSG_CAP);
  printf ("          Disable the usage of messages larger than 4096 bytes.\n");
 printf ("          This includes the capability exchange.(Default enabled)\n");
  printf ("  -%c, %s\n", P_C_NO_EXT_MSG_LIBERAL, P_NO_EXT_MSG_LIBERAL);
  printf ("          Reject extended messages if not properly negotiated.\n");
  printf ("  %s\n", P_EXT_MSG_FORCE);
  printf ("          Force sending extended messages regardless if capability\n");
  printf ("          is negotiated. Allows debugging the peer.\n");
  
  // Disconnect time
  printf ("  -%c <time>, %s <time>\n", P_C_DISCONNECT_TIME, P_DISCONNECT_TIME);
  printf ("          The minimum time in seconds the session stays up after\n");
  printf ("          the last update was sent. The real dissconnect time is\n");
  printf ("          somewhere between <tim> and <holdTime> / 3.\n");
  printf ("          A time of 0 \"zero\" disables theautomatic disconnect.\n");
  
  // Pre-compute EC_KEY
  printf ("  -%c, %s\n", P_C_NO_PL_ECKEY, P_NO_PL_ECKEY);
  printf ("          Disable pre-computation of EC_KEY structure during\n");
  printf ("          loading of the private and public keys.\n");

  // Binary Input file
  printf ("  -%c <filename>, %s <filename>\n", P_C_BINFILE, P_BINFILE);
  printf ("          The filename containing the binary input data. Here \n");
  printf ("          only the first configured session will be used.\n");

  // Binary Output file
  printf ("  -%c <filename>, %s <filename>\n", P_C_OUTFILE, P_OUTFILE);
  printf ("          The filename where to write the output to - Here only\n");
  printf ("          the first configures session will be used.\n");
  printf ("          Requires GEN mode!!\n");

  // Binary Output file
  printf ("  -%c, %s\n", P_C_APPEND_OUT, P_APPEND_OUT);
  printf ("          If specified, the generated data will be appended to\n");
  printf ("          given outfile. In case the outfile does not exist, a\n");
  printf ("          new one will be generated.\n");
  printf ("          Requires GEN mode!!\n");
  
  // Use Maximum number of updates 
  printf ("  -%c, %s\n", P_C_MAX_UPD, P_MAX_UPD);
  printf ("          Allows to restrict the number of updates generated.\n");
  
  // -C <config-file> - Generate a config file.
  printf ("  -%c <filename>\n", P_C_CREATE_CFG_FILE);
  printf ("          Generate a configuration file. The configuration file\n");
  printf ("          uses the given setup (parameters, configuration file)\n");
  printf ("          or generates a sample file if no configuration is\n");
  printf ("          specified.\n");
  
  printf ("\n Configuration file only parameters:\n");

  // BGPSEC Configuration
  // Enable and disable BGPSEC IPv4 Receive
  printf ("  %s\n", P_CFG_BGPSEC_V4_R);
  printf ("          Specify if bgpsec-io can receive IPv4 BGPSEC traffic.\n");
  printf ("          Default: true\n");
  // Enable and disable BGPSEC IPv4 Send
  printf ("  %s\n", P_CFG_BGPSEC_V4_S);
  printf ("          Specify if bgpsec-io can send IPv4 BGPSEC traffic.\n");
  printf ("          Default: true\n");
  // Enable and disable BGPSEC IPv6 Receive
  printf ("  %s\n", P_CFG_BGPSEC_V6_R);
  printf ("          Specify if bgpsec-io can receive IPv6 BGPSEC traffic.\n");
  printf ("          Default: true\n");
  // Enable and disable BGPSEC IPv6 Send
  printf ("  %s\n", P_CFG_BGPSEC_V6_S);
  printf ("          Specify if bgpsec-io can send IPv6 BGPSEC traffic.\n");
  printf ("          Default: false\n");
  
  // signature_generation
  printf ("  %s\n", P_CFG_SIG_GENERATION);
  printf ("          Specify the signature generation mode:\n");
  printf ("          mode CAPI: Use CAPI to sign the updates.\n");
  printf ("          mode BIO: Use internal signature algorithm (default).\n");
  printf ("          mode BIO-K1: Same as BIO except it uses a static k.\n");
  printf ("          mode BIO-K2: Same as BIO except it uses a static k.\n");
  printf ("          The signature modes BIO-K1 and BIO-K2 both use a k \n");
  printf ("          which is specified in RFC6979 Section A.2.5\n");
  printf ("          BIO-K1 uses k for SHA256 and msg=sample.\n");
  printf ("           %s.\n", SM_BIO_K1_STR);
  printf ("          BIO-K2 uses k for SHA256 and msg=test.\n");
  printf ("           %s.\n", SM_BIO_K2_STR);
  
  // Force extended length for BGPSEC path attribtue.
  printf ("  %s\n", P_CFG_ONLY_EXTENDED_LENGTH);
  printf ("          Force usage of extended length also for BGPSEC\n");
  printf ("          path attributes with a length of less than 255 bytes.\n");
  
  // Fake signature portion
  printf ("  %s\n", P_CFG_NULL_SIGNATURE_MODE);
  printf ("          Specify what to do in case no signature can be\n");
  printf ("          generated. Example: no key information is found.\n");
  printf ("          Valid values are (%s|%s|%s).\n", 
          P_TYPE_NSM_DROP, P_TYPE_NSM_FAKE, P_TYPE_NSM_BGP4);

  // fake signature
  printf ("  %s\n", P_CFG_FAKE_SIGNATURE);
  printf ("          This string contains the fake signature in hex format.\n");
  printf ("          The signature must not be longer than %i bytes.\n", 
          MAX_SIG_BYTE_SIZE);
  printf ("          (2 HEX characters equals one byte!).\n");

  // fake ski
  printf ("  %s\n", P_CFG_FAKE_SKI);
  printf ("          This string contains the fake ski for not found keys.\n");
  printf ("          The SKI MUST consist of %i bytes.\n", 
          SKI_LENGTH);
  printf ("          (2 HEX characters equals one byte!).\n\n");

  // print... Portion
  printf ("  %s\n", P_CFG_PRINT_ON_RECEIVE);
  printf ("          Each BGP update packet send out will be printed on\n");
  printf ("          standard output in WireShark form.\n");
  printf ("          Use this setting for debug only!!\n");
  
  printf ("  %s\n", P_CFG_PRINT_ON_SEND);
  printf ("          Each BGP packet received out will be printed on\n");
  printf ("          standard output in WireShark form.\n");
  printf ("          Use this setting for debug only!!\n");

  printf ("  %s\n", P_CFG_PRINT_POLL_LOOP);
  printf ("          Print information each time the poll loop runs.\n");
  
  printf ("  %s\n", P_CFG_PRINT_CAPI_ON_INVALID);
  printf ("          Print status information on validation result invalid.\n");
  printf ("          This setting only affects the CAPI mode.\n");
  
  printf ("\n");
  printf ("%s Version %s\nDeveloped 2015-2016 by Oliver Borchert ANTD/NIST\n", 
          PACKAGE_NAME, PACKAGE_VERSION);
  printf ("Send bug reports to %s\n\n", PACKAGE_BUGREPORT);
}

/**
 * Display the version number.
 * 
 * @since 0.2.0.2
 */
void printVersion()
{
  printf ("%s Version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
}

/**
 * Translate the given parameter in a one character parameter if possible or 0 
 * if not known.
 * 
 * @param argument The argument
 * 
 * @return the one character replacement or 0 "zero"
 */
char getShortParam(char* argument)
{
  char retVal = 0;

  if (*argument == '-')
  {
    if (strlen(argument) == 2)
    {
      argument++;
      retVal = *argument;
    }  
    else if (strcmp(argument, P_HELP) == 0)        { retVal = P_C_HELP_1; }
    else if (strcmp(argument, P_VERSION) == 0)     { retVal = P_C_VERSION; }
    else if (strcmp(argument, P_SKI_FILE) == 0)    { retVal = P_C_SKI_FILE; }
    else if (strcmp(argument, P_SKI_LOC) == 0)     { retVal = P_C_SKI_LOC; }
    else if (strcmp(argument, P_TYPE) == 0)        { retVal = P_C_TYPE; }
    else if (strcmp(argument, P_BGP_IDENT) == 0)   { retVal = P_C_BGP_IDENT; }
    else if (strcmp(argument, P_MY_ASN) == 0)      { retVal = P_C_MY_ASN; }
    else if (strcmp(argument, P_PEER_AS) == 0)     { retVal = P_C_PEER_AS; }
    else if (strcmp(argument, P_PEER_IP) == 0)     { retVal = P_C_PEER_IP; }
    else if (strcmp(argument, P_CONFIG) == 0)      { retVal = P_C_CONFIG; }
    else if (strcmp(argument, P_UPD_PARAM) == 0)   { retVal = P_C_UPD_PARAM; }
    else if (strcmp(argument, P_BINFILE) == 0)     { retVal = P_C_BINFILE; }
    else if (strcmp(argument, P_OUTFILE) == 0)     { retVal = P_C_OUTFILE; }
    else if (strcmp(argument, P_APPEND_OUT) == 0)  { retVal = P_C_APPEND_OUT; }
    else if (strcmp(argument, P_NO_MPNLRI) == 0)   { retVal = P_C_NO_MPNLRI; }
    else if (strcmp(argument, P_NO_EXT_MSG_CAP) == 0) 
         { retVal = P_C_NO_EXT_MSG_CAP; }
    else if (strcmp(argument, P_NO_EXT_MSG_LIBERAL) == 0) 
         { retVal = P_C_NO_EXT_MSG_LIBERAL; }
    else if (strcmp(argument, P_NO_PL_ECKEY) == 0) { retVal = P_C_NO_PL_ECKEY; }
    else if (strcmp(argument, P_CAPI_CFG) == 0)    { retVal = P_C_CAPI_CFG; }
    else if (strcmp(argument, P_MAX_UPD) == 0)     { retVal = P_C_MAX_UPD; }
  }
  
  return retVal;
}

/**
 * This function verifies that all necessary data is provided to tun the BGP
 * daemon.
 * 
 * @param params The parameters / configurations.
 * 
 * @return true if BGP daemon can be started, false if not. 
 */
bool checkBGPConfig(PrgParams* params)
{
  bool bgpConf = false;
  BGP_SessionConf* config = &params->bgpConf;

  if (params->type == OPM_BGP)
  {
    bgpConf = config->bgpIdentifier && config->asn && config->peerAS 
              && config->peer_addr.sin_addr.s_addr 
              && config->peer_addr.sin_port;    
    
    // Check if data is coming in through pipe or via scripted updates
    if (!feof(stdin) || !isStackEmpty(&params->updateStack)) 
    {
        // Data comes through the stream, maybe additional data also comes from
        // configuration but main data comes from stream.
        bgpConf = bgpConf 
                  && params->keyLocation && params->skiFName;       
    }
  }

  return bgpConf;
}

/**
 * Writes an error message SESS_ERR or SESS_ERR_1. The later one is used in case
 * error is NOT NULL.
 * 
 * @param params The parameter to set the error in
 * @param session The session of the error
 * @param paramName The name of the parameter itself
 * @param error The error string itself - Uses SESS_ERR_1 if NOT NULL
 */
static void _setConfigSessErr(PrgParams* params, int session, 
                                   char* paramName, char* error)
{
  if (error == NULL)
  {
    sprintf((char*)&params->errMsgBuff, SESS_ERR, session, paramName);
  }
  else
  {
    sprintf((char*)&params->errMsgBuff, SESS_ERR_1, session, paramName, error);    
  }
}

/**
 * Generate the UpdateData instance from the given string in the format given 
 * format prefix[,[as-path]]
 * 
 * @param prefix_path the given path
 * @param params the program params
 * 
 * @return the update data or NULL in case of an error. 
 */
UpdateData* createUpdate(char* prefix_path, PrgParams* params)
{  
  IPPrefix    prefix;
  UpdateData* update = NULL;
  char  pfxStr[IP_STRING];
  char* pathStr = "\0";
  int   psStrLen = 0;
  int   pfxStrLen = 0;
  bool  startProcess = true;
  
  // Only if the handed path is not null
  if (prefix_path != NULL)
  {
    // Now find separation of prefix and string
    char* sepPos = strchr(prefix_path, ',');
    
    // Separation found.
    if (sepPos != NULL)
    {
      pfxStrLen = (int)(sepPos - prefix_path);
      
      // Set the pointer to the beginning of the path
      pathStr = sepPos+1;
      psStrLen  = strlen(pathStr);
    }
    else
    {
      // BUGFIX #923
      if (strchr(prefix_path, ' ') != NULL)
      {
        // What this means is that it is very possible that the ',' is missing.
        // The update might be '1.2.3.4/32 10' but should be '1.2.3.4/32, 10'.
        // The downside is that blanks are only allowed if a separator is found. 
        // But tit solves the issue if trying to figure out why updates are
        // not being delivered with the correct path because a comma is missing.
        startProcess = false;
        printf("WARNING: Update '%s' incomplete - separator between prefix and "
               "path is missing!\n", prefix_path);
      }
      pfxStrLen = strlen(prefix_path);
    }
  }
  
  if (startProcess)
  { 
    // BZ892: In case a "," is missing between the configuration of two 
    // independent updates within the configuration script, the pathStr will
    // contain a second prefix. Check for it and if it exists, trigger an error 
    // by setting pfxStrLen to 0 "zero"
    char* slash1 = index(prefix_path, '/');
    char* slash2 = rindex(prefix_path, '/');
    if (slash1 != slash2 || slash1 == NULL)
    {
      pfxStrLen = 0; // Trigger the error.
    }
      
    if (pfxStrLen != 0)  
    { 
      // set the path
      update = malloc(sizeof(UpdateData));
      memset (update, 0, sizeof(UpdateData));
      // Allocate memory for the path string and set the path
      update->pathStr = malloc(psStrLen+1);
      snprintf(update->pathStr, psStrLen+1, "%s", pathStr);
              
      // Now generate the prefix.
        // Copy the prefix into its own string
      memset(&pfxStr, '\0', IP_STRING);
      memcpy(&pfxStr, prefix_path, pfxStrLen);
      memset(&update->prefixTpl, 0, sizeof(BGPSEC_V6Prefix));      
      if (strToIPPrefix((char*)&pfxStr, &prefix))
      {
        // the required size to store the padded prefix.
        update->prefixTpl.prefix.safi   = SAFI_UNICAST;
        update->prefixTpl.prefix.length = prefix.length;

        switch (prefix.ip.version)
        {
          case ADDR_IP_V6:
          case AFI_V6:
            update->prefixTpl.prefix.afi = htons(AFI_V6);
            memcpy(update->prefixTpl.addr, prefix.ip.addr.v6.u8, 
                   sizeof(prefix.ip.addr.v6.u8));
            break;
          case ADDR_IP_V4:
          case AFI_V4:
          default:
            update->prefixTpl.prefix.afi = htons(AFI_V4);
            memcpy(update->prefixTpl.addr, prefix.ip.addr.v4.u8, 
                   sizeof(prefix.ip.addr.v4.u8));
            break;
        }        
      }
      else
      { 
        _setErrMsg(params, "Invalid Prefix specification!"); 
        freeUpdateData(update);
        update = NULL;
      }
    }
    else
    {
      _setErrMsg(params, "Invalid path specification!");
    }    
  }
  
  return update;  
}

/**
 * Read the updates from the given configuration list element and push them on
 * the stack. The updates read in are copied into newly allocates strings.
 * These strings will be pushed and need to be freed later by the consumer!!
 * 
 * @param list the configuration list
 * @param stack the stack where to add the updates to 
 * @param params the program parameters - mainly for the error string.
 * 
 * @return true if all updates could be read, otherwise false.
 */
bool _readUpdates(const config_setting_t* updates, Stack* stack, 
                       PrgParams* params)
{
  bool  retVal = updates != NULL && stack != NULL && params != NULL;  
  char* strVal = NULL;
  int   updCt = config_setting_length(updates);
  int   idx;
  
  for (idx = 0; (idx < updCt) && retVal; idx++)
  {
    strVal = (char*)config_setting_get_string_elem(updates, idx);
    if (strVal)
    {
      UpdateData* updateData = createUpdate(strVal, params);
      if (updateData)
      {
        fifoPush(stack, updateData);      
      }
      else if (params->errMsgBuff[0] != '\0')
      {
        retVal = false;
      }
    }
  }
  
  return retVal;
}

/**
 * Fill the given address with the correct ip address and port. The port is
 * given in host format but will be stored in network format.
 * 
 * @param ipStr The IP address - will only be set if not NULL
 * @param port The port in host form, will only be set if > 0
 * @param addr The sockaddr_in address that will be filled.
 */
void _setIPAddress(const char* ipStr, u_int16_t port, struct sockaddr_in* addr)
{
  addr->sin_family      = AF_INET;
  if (ipStr != NULL)
  {
    addr->sin_addr.s_addr = inet_addr(ipStr);
  }
  if (port > 0)
  {
    addr->sin_port = htons(port); 
  }
}

/**
 * Read the given configuration file and set the params.
 * 
 * @param params The parameters
 * 
 * @return true if the configuration file could be read.
 */
bool readConfig(PrgParams* params)
{
  static config_t          cfg;
  static config_setting_t* cfgHlp;
  static config_setting_t* session;
  static config_setting_t* sessVal;
  static config_setting_t* updates;

  int sessCt  = 0;
  int sessIdx = 0;

  IPAddress ipAddr;
  
  const char* strVal  = NULL;
  LCONFIG_INT intVal  = 0;
  
  PrgParams*  sParam  = params; // used to allows later on an easy transition to 
                             // multi sessions.
  Stack*      sStack  = &params->updateStack;

  struct stat st;
  if (stat(params->cfgFile, &st) != 0)
  {
    printf ("ERROR: Configuration file '%s' not found!\n", params->cfgFile);
    return false;
  }
 
  // Initialize libconfig
  config_init(&cfg);

  // Try to parse the configuration file
  int cfgFile = config_read_file(&cfg, params->cfgFile);
  if (cfgFile == CONFIG_TRUE)
  {    
    if (config_lookup_string(&cfg, P_CFG_SKI_FILE, &strVal) == CONFIG_TRUE)
    {
      snprintf((char*)&params->skiFName, FNAME_SIZE, "%s", strVal);
    }

    if (config_lookup_string(&cfg, P_CFG_SKI_LOC, &strVal) == CONFIG_TRUE)
    {
      snprintf((char*)params->keyLocation, FNAME_SIZE, "%s", strVal);
    }

    if (config_lookup_string(&cfg, P_CFG_CAPI_CFG, &strVal) == CONFIG_TRUE)
    {
      snprintf((char*)params->capiCfgFileName, FNAME_SIZE, "%s", strVal);
    }
    
    if (config_lookup_string(&cfg, P_CFG_BINFILE, &strVal) == CONFIG_TRUE)
    {
      snprintf((char*)params->binInFile, FNAME_SIZE, "%s", strVal);
    }
    
    if (config_lookup_string(&cfg, P_CFG_OUTFILE, &strVal) == CONFIG_TRUE)
    {
      snprintf((char*)params->binOutFile, FNAME_SIZE, "%s", strVal);
    }
    
    if (config_lookup_bool(&cfg, P_CFG_APPEND_OUT, (int*)&intVal) ==CONFIG_TRUE)
    {
      params->appendOut = (bool)intVal;
    }
    
    if (config_lookup_bool(&cfg, P_CFG_PL_ECKEY, (int*)&intVal) == CONFIG_TRUE)
    {
      params->preloadECKEY = (bool)intVal;
    }
    
    if (config_lookup_bool(&cfg, P_CFG_ONLY_EXTENDED_LENGTH, (int*)&intVal) == CONFIG_TRUE)
    {
      params->onlyExtLength = (bool)intVal;
    }
    
    if (config_lookup_int(&cfg, P_CFG_MAX_UPD, &intVal) == CONFIG_TRUE)
    {
      params->maxUpdates = intVal != 0 ? (u_int32_t)intVal : MAX_UPDATES;
    }
    
    if (config_lookup_bool(&cfg, P_CFG_ONLY_EXTENDED_LENGTH, (int*)&intVal) == CONFIG_TRUE)
    {
      params->onlyExtLength = (bool)intVal;
    }

    if (config_lookup_string(&cfg, P_CFG_TYPE, &strVal) == CONFIG_TRUE)
    {
      if (strcmp(strVal, P_TYPE_BGP) == 0)
      {
        params->type = OPM_BGP;
      } 
      else if (strcmp(strVal, P_TYPE_CAPI) == 0)
      {
        params->type = OPM_CAPI;
      } 
      else if (strcmp(strVal, P_TYPE_GENB) == 0)
      {
        params->type = OPM_GEN_B;
      }
      else if (strcmp(strVal, P_TYPE_GENC) == 0)
      {
        params->type = OPM_GEN_C;
      }
      else
      {
        sprintf(params->errMsgBuff, "Invalid 'type' %s", strVal);        
      }
    }
    
    cfgHlp = config_lookup(&cfg, P_CFG_SESSION);
    if (cfgHlp && params->errMsgBuff[0] == '\0')
    {
      if (config_setting_is_list(cfgHlp))
      {        
        // Configure all sessions.
        sessCt = config_setting_length(cfgHlp);
        for (sessIdx = 0; sessIdx < sessCt; sessIdx++)
        {
#ifndef SUPPORT_MULTI_SESSION
          if (sessIdx > 0)
          {
            printf("Multi-Sessions are currently not supported, skip all but "
                   "the first session configuration!\n");   
            sessIdx = sessCt;
            break;
          }
#endif
          session = config_setting_get_elem(cfgHlp, sessIdx);
          // My ASN
          sessVal = config_setting_get_member(session, P_CFG_MY_ASN);
          if (sessVal == NULL)
            { _setConfigSessErr(params, sessIdx, P_CFG_MY_ASN, NULL); break; }
          sParam->bgpConf.asn = (u_int32_t)config_setting_get_int(sessVal);
          
          // BGP Identifier
          strVal = NULL;
          sessVal = config_setting_get_member(session, P_CFG_BGP_IDENT);
          if (sessVal == NULL)
            { _setConfigSessErr(params, sessIdx, P_CFG_BGP_IDENT, NULL); break;}
          strVal = config_setting_get_string(sessVal);
          if (strVal != NULL)
          {
            strToIPAddress(strVal, &ipAddr); 
            if (ipAddr.version == ADDR_IP_V4)
            {
              sParam->bgpConf.bgpIdentifier = ntohl(ipAddr.addr.v4.u32);            
            }
            else
            {
              _setConfigSessErr(params, sessIdx, P_CFG_BGP_IDENT, 
                                " - BGP identifier MUST be an IPv4 address!");
              break;
              // Here we need to exit with break
            }
          }

          // The hold timer          
          sessVal = config_setting_get_member(session, P_CFG_HOLD_TIME);
          sParam->bgpConf.holdTime = sessVal == NULL
                             ? DEF_HOLD_TIME
                             : (u_int32_t)config_setting_get_int(sessVal);
                  
          // The disconnectTime
          sessVal = config_setting_get_member(session, P_CFG_DISCONNECT_TIME);
          sParam->bgpConf.disconnectTime = sessVal == NULL
                             ? DEF_DISCONNECT_TIME
                             : (u_int32_t)config_setting_get_int(sessVal);
          
          // Peer AS
          sessVal = config_setting_get_member(session, P_CFG_PEER_AS);
          if (sessVal == NULL)
            { _setConfigSessErr(params, sessIdx, P_CFG_PEER_AS, NULL); break; }
          sParam->bgpConf.peerAS = (u_int32_t)config_setting_get_int(sessVal);
          
          // Peer Port 
          sessVal = config_setting_get_member(session, P_CFG_PEER_PORT);
          intVal = sessVal == NULL ? DEF_PEER_PORT
                                   : (u_int32_t)config_setting_get_int(sessVal);
          // Peer IP
          sessVal = config_setting_get_member(session, P_CFG_PEER_IP);
          if (sessVal == NULL)
            { _setConfigSessErr(params, sessIdx, P_CFG_PEER_IP, NULL); break; }
          strVal = (char*)config_setting_get_string(sessVal);
          // Now set the peer information (port and address)
          _setIPAddress(strVal, intVal, &sParam->bgpConf.peer_addr);

          // Read MPNLRI
          sessVal = config_setting_get_member(session, P_CFG_MPNLRI);
          if (sessVal != NULL)
          {
            params->bgpConf.useMPNLRI = config_setting_get_bool(sessVal);
            // Added with 0.2.0.2 - This caused invalid packets being send.
            if (!params->bgpConf.useMPNLRI)
            {
                printf("WARNING: Attibue deprecated!!!\n"
                       "Disabling useMPNLRI will produce invalid "
                       "BGPSEC updates - This should only be used for "
                       "test purpose - if at all. This setting will be "
                       "removed in a later update!!\n");
            }
          }
          
          // Read Ext Message Capability
          sessVal = config_setting_get_member(session, P_CFG_EXT_MSG_CAP);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.extMsgSupp = 
                                               config_setting_get_bool(sessVal);
          }

          // Read Ext Message Capability Liberal Processing
          sessVal = config_setting_get_member(session, P_CFG_EXT_MSG_LIBERAL);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.extMsgLiberal = 
                                               config_setting_get_bool(sessVal);
          }
          
          // Allows to enable forcing to send extended message regardless of
          // capability negotiation. For TESTING PEER ONLY
          sessVal = config_setting_get_member(session, P_CFG_EXT_MSG_FORCE);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.extMsgForce = 
                                               config_setting_get_bool(sessVal);
          }          
          
          // Read BGPSEC Configuration
          // Enable and disable BGPSEC IPv4 Receive          
          sessVal = config_setting_get_member(session, P_CFG_BGPSEC_V4_R);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.bgpsec_rcv_v4 = 
                                               config_setting_get_bool(sessVal);
          }
          // Enable and disable BGPSEC IPv4 Send
          sessVal = config_setting_get_member(session, P_CFG_BGPSEC_V4_S);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.bgpsec_snd_v4 = 
                                               config_setting_get_bool(sessVal);
          }
          // Enable and disable BGPSEC IPv6 Receive
          sessVal = config_setting_get_member(session, P_CFG_BGPSEC_V6_R);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.bgpsec_rcv_v6 = 
                                               config_setting_get_bool(sessVal);
          }
          // Enable and disable BGPSEC IPv6 Send
          sessVal = config_setting_get_member(session, P_CFG_BGPSEC_V6_S);
          if (sessVal != NULL)
          {
            params->bgpConf.capConf.bgpsec_snd_v6 = 
                                               config_setting_get_bool(sessVal);
          }          
          
          // Read Algorithm Settings
          // AlgoID
          sessVal = config_setting_get_member(session, P_CFG_ALGO_ID);
          intVal = sessVal == NULL ? DEF_ALGO_ID
                                   : (u_int32_t)config_setting_get_int(sessVal);
          params->bgpConf.algoParam.algoID = (u_int8_t)intVal;          
          
          sessVal = config_setting_get_member(session, P_CFG_SIG_GENERATION);
          if (sessVal != NULL)
          { // Here we do it a bit different, don't throw an error 
            strVal = (char*)config_setting_get_string(sessVal);
            if (strcmp(strVal, P_TYPE_SIGMODE_CAPI) == 0)
            {
              params->bgpConf.algoParam.sigGenMode = SM_CAPI;
            }
            else if (strcmp(strVal, P_TYPE_SIGMODE_BIO) == 0)
            {
              params->bgpConf.algoParam.sigGenMode = SM_BIO;
            }
            else if (strcmp(strVal, P_TYPE_SIGMODE_BIO_K1) == 0)
            {
              params->bgpConf.algoParam.sigGenMode = SM_BIO_K1;
            }
            else if (strcmp(strVal, P_TYPE_SIGMODE_BIO_K2) == 0)
            {
              params->bgpConf.algoParam.sigGenMode = SM_BIO_K2;
            }
            else
            {
              sprintf(params->errMsgBuff, "Invalid 'type' %s", strVal);        
            }
          }          
          
          sessVal = config_setting_get_member(session, P_CFG_NULL_SIGNATURE_MODE);
          if (sessVal != NULL)
          { // Here we do it a bit different, don't throw an error 
            strVal = (char*)config_setting_get_string(sessVal);
            if (strcmp(strVal, P_TYPE_NSM_DROP) == 0)
            {
              params->bgpConf.algoParam.ns_mode = NS_DROP;
            } else if (strcmp(strVal, P_TYPE_NSM_BGP4) == 0)
            {
              params->bgpConf.algoParam.ns_mode = NS_BGP4;
            } else if (strcmp(strVal, P_TYPE_NSM_FAKE) == 0)
            {
              params->bgpConf.algoParam.ns_mode = NS_FAKE;

              sessVal = config_setting_get_member(session, P_CFG_FAKE_SIGNATURE);
              if (sessVal == NULL)          
                { _setConfigSessErr(params, sessIdx, P_CFG_FAKE_SIGNATURE, NULL); break; }
              strVal = (char*)config_setting_get_string(sessVal);
              intVal = (u_int8_t)strlen(strVal);
              intVal = au_hexStrToBin((char*)strVal, 
                       params->bgpConf.algoParam.fake_signature, 
                       intVal < MAX_SIG_BYTE_SIZE ? intVal : MAX_SIG_BYTE_SIZE);
              params->bgpConf.algoParam.fake_sigLen = (u_int8_t)intVal;

              sessVal = config_setting_get_member(session, P_CFG_FAKE_SKI);
              if (sessVal == NULL)          
                { _setConfigSessErr(params, sessIdx, P_CFG_FAKE_SKI, NULL); break; }
              strVal = (char*)config_setting_get_string(sessVal);
              intVal = au_hexStrToBin((char*)strVal, 
                                      params->bgpConf.algoParam.fake_key.ski, 
                                      SKI_HEX_LENGTH);
            } else
            {
              sprintf(params->errMsgBuff, "Invalid 'type' %s", strVal);        
            }
          }

          // Read print on receive
          sessVal = config_setting_get_member(session, P_CFG_PRINT_ON_RECEIVE);
          if (sessVal != NULL)
          {
            bool bVal = config_setting_get_bool(sessVal);
            int idx;
            for (idx = 0; idx < PRNT_MSG_COUNT; idx++)
            {
              params->bgpConf.printOnReceive[idx] = bVal;
            }
          }

          // Read print on send
          sessVal = config_setting_get_member(session, P_CFG_PRINT_ON_SEND);
          if (sessVal != NULL)
          {
            bool bVal = config_setting_get_bool(sessVal);
            int idx;
            for (idx = 0; idx < PRNT_MSG_COUNT; idx++)
            {
              params->bgpConf.printOnSend[idx] = bVal;
            }
          }

          // Read print poll loop
          sessVal = config_setting_get_member(session, P_CFG_PRINT_POLL_LOOP);
          if (sessVal != NULL)
          {
            params->bgpConf.printPollLoop = config_setting_get_bool(sessVal);
          }

          // Read print status on invalid
          sessVal = config_setting_get_member(session, 
                                              P_CFG_PRINT_CAPI_ON_INVALID);
          if (sessVal != NULL)
          {
            params->bgpConf.printOnInvalid = config_setting_get_bool(sessVal);
          }
          
          // Read session Updates
          updates = config_setting_get_member(session, P_CFG_UPD_PARAM);
          if (updates)
          {
            if (config_setting_is_list(updates))
            {
              // in case of an error the message is written in params and break 
              // is not necessary because the loop ends here anyway
              _readUpdates(updates, sStack, params);
            }
            else
            {
              _setConfigSessErr(params, sessIdx, P_CFG_UPD_PARAM, 
                              " must be a comma separated list!!!");
              // break not necessary because the loop ends here anyway
            }
          }        
        }
      }
    
      if (params->errMsgBuff[0] == '\0')
      {
        // Now read global updates
        updates = config_lookup(&cfg, P_CFG_UPD_PARAM);
        if (updates)
        {
          if (config_setting_is_list(updates))
          {
            _readUpdates(updates, &params->updateStack, params);
          }
          else
          {
            _setConfigSessErr(params, sessIdx, P_CFG_UPD_PARAM, 
                              " must be a comma separated list!!!");
          }
        }
      }
    }
  }
  else
  {
    sprintf(params->errMsgBuff, "Error in configuration[%i]: '%s'%c", 
            cfg.error_line, cfg.error_text, '\0');
  }

  config_destroy(&cfg);

  return true;
}

/**
 * Initialize the params and set default values. Here we also set the default 
 * capabilities we provide (such as send bgpsec V4 and V6)
 * 
 * @param params The parameters object.
 * 
 */
void initParams(PrgParams* params)
{
  int idx;
  memset(params, 0, sizeof(PrgParams));
  
  // Now initialize what should not be 0 - false - NULL
  snprintf((char*)&params->skiFName, FNAME_SIZE, "%s", DEF_SKIFILE);
  snprintf((char*)&params->keyLocation, FNAME_SIZE, "%s", DEF_KEYLOCATION);
  params->bgpConf.useMPNLRI           = true;
  params->preloadECKEY                = true;
  params->onlyExtLength               = true;
  params->appendOut                   = false;
  params->bgpConf.printPollLoop       = false;
  params->bgpConf.printOnInvalid      = false;
  for (idx = 0; idx < PRNT_MSG_COUNT; idx++)
  {
    params->bgpConf.printOnSend[idx]    = false;
    params->bgpConf.printOnReceive[idx] = false;
  }
  
  params->maxUpdates = MAX_UPDATES;
  
  memset(&params->bgpConf.algoParam, 0, sizeof (AlgoParam)); 
  // The following line is normally not needed, I just add it in case the SM_BIO
  // value will be modified and SM_BIO is the default value.
  params->bgpConf.algoParam.sigGenMode = SM_BIO;
  
  // Set all capabilities to true
  memset(&params->bgpConf.capConf, 1, sizeof(BGP_Cap_Conf));
  // Turn capabilities selectively off
  params->bgpConf.capConf.route_refresh = false;
  // Turn capabilities selectively off
  params->bgpConf.capConf.bgpsec_snd_v6 = false;
  // Turn off TEST setting for forcing ext message sending without ext 
  // capability being negotiated
  params->bgpConf.capConf.extMsgForce = false;
  
  initStack(&params->updateStack);
}

/**
 * Parse the given program parameter
 * 
 * @param params Stores the settings found in the program arguments
 * @param argc The number of arguments
 * @param argv the array containing the program arguments
 * 
 * @return 1 for success, 0 for stop (help), -1 for error
 */
int parseParams(PrgParams* params, int argc, char** argv)
{
  // Set the default parameters.  
  // Read Parameters
  int idx = 1;
  UpdateData* update = NULL;
  int retVal = 1;
  bool loadCfgScript = false;

  // first check for help
  while (idx < argc && params->errMsgBuff[0] == '\0')
  {
    switch (getShortParam(argv[idx]))
    {
      case P_C_HELP:
      case P_C_HELP_1:
      case P_C_HELP_2:
        printSyntax();
        idx = argc;
        retVal = 0;
        break;
      case P_C_VERSION:
        printVersion();
        retVal = 0;
        break;
      case P_C_CONFIG:
        // check here speeds up the rest.
        if (++idx >= argc) 
          { _setErrMsg(params, "Configuration file not specified!"); break; }
        loadCfgScript = true;
        sprintf((char*)&params->cfgFile, "%s%c", argv[idx], '\0');
        idx = argc; // skip the rest of the loop.
        break;        
      default:
        break;
    }    
    idx++;
  }
  
  // first check for configuration file - skip all parameters except the 
  // configuration file.
  if (loadCfgScript && (retVal == 1))
  {
    // Load the configuration file.s
    if (!readConfig(params) && params->errMsgBuff[0] == '\0')
    {
      sprintf((char*)&params->errMsgBuff, 
              "Error while processing configuration file %s'%c", 
              params->cfgFile, '\0');        
    }
  }

  // Now parse again the parameters but skip the configuration file this time
  idx = 1;
  while (idx < argc && params->errMsgBuff[0] == '\0')
  {
    // First check the long letter parameters:
    update = NULL; // need to set to null to indicate a free in case of an error
    switch (getShortParam(argv[idx]))
    {
      case P_C_CONFIG:
        idx++; // needed to skip the configuration file name.
      case P_C_HELP:
      case P_C_HELP_1:
      case P_C_HELP_2:
      case P_C_VERSION:
        // skip this one, was already processed.
        break;
        
      case P_C_UPD_PARAM:
        if (++idx >= argc) 
          { _setErrMsg(params, "Not enough Parameters!"); break; }      

        update = createUpdate(argv[idx], params);
        if (update != NULL)
        {  
          // Use fifo put to allow the updates to be send in the order read. 
          fifoPush(&params->updateStack, update);
        }
        update = NULL; // Update is stored in stack
        break;

      case P_C_SKI_FILE:
        if (++idx >= argc) 
          { _setErrMsg(params, "SKI file not specified!"); break; }
        sprintf((char*)&params->skiFName, "%s\n", argv[idx]);
        break;

      case P_C_SKI_LOC:
        if (++idx >= argc) 
          { _setErrMsg(params, "Key location not specified!"); break; }
        sprintf((char*)&params->keyLocation, "%s%c", argv[idx], '\0'); 
        break;

      case P_C_CAPI_CFG:
        if (++idx >= argc) 
          { _setErrMsg(params, "SrxCryptoAPI configfile not specified!"); break; }
        sprintf((char*)&params->capiCfgFileName, "%s\n", argv[idx]);
        break;
        
      case P_C_TYPE:
        if (++idx >= argc) 
          { _setErrMsg(params, "Type missing!"); break; }
        if (strcmp(argv[idx], P_TYPE_BGP) == 0)
        {
          params->type = OPM_BGP;
        }
        else if (strcmp(argv[idx], P_TYPE_CAPI) == 0)
        {
          params->type = OPM_CAPI;          
        }
        else if (strcmp(argv[idx], P_TYPE_GENB) == 0)
        {
          params->type = OPM_GEN_B;          
        }
        else if (strcmp(argv[idx], P_TYPE_GENC) == 0)
        {
          params->type = OPM_GEN_C;          
        }
        else { _setErrMsg(params, "Invalid running type!"); break; }          
        break;
        
      case P_C_MAX_UPD:
        if (++idx >= argc) 
          { _setErrMsg(params, "Maximum number of updates missing!"); break; }
        params->maxUpdates = atoi(argv[idx]);
        if (params->maxUpdates == 0)
        {
          params->maxUpdates = MAX_UPDATES;
        }
        break;        

      case P_C_MY_ASN:
        if (++idx >= argc) 
          { _setErrMsg(params, "Own AS number missing!"); break; }
        params->bgpConf.asn = atoi(argv[idx]);
        break;

      case P_C_BGP_IDENT:
        if (++idx >= argc) 
          { _setErrMsg(params, "BGP Identifier missing!"); break; }
        IPAddress ipAddr;
        if (!strToIPAddress(argv[idx], &ipAddr)) 
          { _setErrMsg(params, "Invalid BGP identifier!"); break; };
        params->bgpConf.bgpIdentifier = ipAddr.addr.v4.u32;
        break;

      case P_C_PEER_AS:
        if (++idx >= argc) 
        { _setErrMsg(params, "Peer AS not specified!"); break; }
        params->bgpConf.peerAS = atoi(argv[idx]);
        break;

      case P_C_PEER_IP:
        if (++idx >= argc) 
          { _setErrMsg(params, "Peer IP not specified!"); break; }
        _setIPAddress(argv[idx], 0, &params->bgpConf.peer_addr);
        break;
        
      case P_C_PEER_PORT:
        if (++idx >= argc) 
        { _setErrMsg(params, "Peer port not specified!"); break; }
        _setIPAddress(NULL, atoi(argv[idx]), &params->bgpConf.peer_addr);
        break;
        
      case P_C_NO_MPNLRI:
        params->bgpConf.useMPNLRI = false;
        break;
        
      case P_C_NO_EXT_MSG_CAP:
        params->bgpConf.capConf.extMsgSupp = false;
        break;

      case P_C_NO_EXT_MSG_LIBERAL:
        params->bgpConf.capConf.extMsgLiberal = false;
        break;
        
      case P_C_NO_PL_ECKEY:
        params->preloadECKEY = false;
        break;
        
      case P_C_DISCONNECT_TIME:
        if (++idx >= argc) 
        { _setErrMsg(params, "Disconnect time not specified!"); break; }
        params->bgpConf.disconnectTime = atoi(argv[idx]);
        break;
        
      case P_C_OUTFILE:
        if (++idx >= argc) 
          { _setErrMsg(params, "Filename for out file missing!"); break; }
        snprintf((char*)&params->binOutFile, FNAME_SIZE, "%s", argv[idx]);
        break;
        
      case P_C_APPEND_OUT:
        params->appendOut = true;
        break;        
        
      case P_C_BINFILE:
        if (++idx >= argc) 
          { _setErrMsg(params, "Filename for binary in-file missing!"); break; }
        snprintf((char*)&params->binInFile, FNAME_SIZE, "%s", argv[idx]);
        break;
        
      case P_C_CREATE_CFG_FILE:
        if (++idx >= argc) 
          { _setErrMsg(params, "Filename for config file missing!"); break; }
        snprintf((char*)&params->newCfgFileName, FNAME_SIZE, "%s", argv[idx]);
        params->createCfgFile = true;
        break;
        
      default:
        // Some parameters do only have a -- setting and no single char option
        if (strcmp(argv[idx], P_EXT_MSG_FORCE) == 0)
        {
          params->bgpConf.capConf.extMsgForce = true;
        }
        else
        {
          snprintf(params->errMsgBuff, PARAM_ERRBUF_SIZE, 
                   "Unknown Parameter '%s'!", argv[idx]);
          idx = argc; // stop further processing.
          printSyntax();
        }
    }

    // something went wrong during the update processing, free the leftover
    if (update != NULL)
    {
      freeUpdateData(update);
    }

    idx++;
  }

  if (params->errMsgBuff[0] != '\0')
  {
    retVal = -1;
  }
  
  return retVal;
}

/** 
 * free the Update data parsed from program parameters. 
 * 
 * @param update The update parameter that has to be freed.
 */
void freeUpdateData(void* upd)
{ 
  if (upd != NULL)
  {
    UpdateData* update = (UpdateData*)upd;
    if (update->pathStr != NULL)
    {
      free(update->pathStr);
      update->pathStr = NULL;
    }
    free(upd);
  }
}

/**
 * Remove all memory allocated within the params structure and set all 
 * allocated memory to "0". If desired free will be called on params as well. 
 * 
 * @param params the params instance
 * @param doFree if true the memory will be freed as well
 */
void cleanupParams(PrgParams* params, bool doFree)
{
  if (!isStackEmpty(&params->updateStack))
  {
    emptyList((List*)&params->updateStack, true, freeUpdateData);
  }
  memset(params, 0, sizeof(PrgParams));
  
  if (doFree)
  {
    free(params);
  }
}