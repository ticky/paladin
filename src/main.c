/*
 *    8888888b.     d8888 888                    888 d8b          R
 *    888   Y88b   d88888 888                    888 Y8P          .E
 *    888    888  d88P888 888                    888               .V
 *    888   d88P d88P 888 888       8888b.   .d88888 888 88888b.    .I      [1.11]
 *    8888888P" d88P  888 888          "88b d88" 888 888 888 "88b    .S
 *    888      d88P   888 888      .d888888 888  888 888 888  888     .I
 *    888     d8888888888 888      888  888 Y88b 888 888 888  888      .O
 *    888    d88P     888 88888888 "Y888888  "Y88888 888 888  888       .N
 *
 *    Enhanced PAL to NTSC Converter for True PSX NTSC Conversion
 *
 *    Changelog :...................................................................
 *     : REVISION 1.0                                         by <Damian Coldbird> :
 *     :     > Initial Release of PALadin - Welcome on Earth! by <Damian Coldbird> :
 *     :     > VMode Type-A Patching Support added            by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.1                                         by <Damian Coldbird> :
 *     :     > VMode Type-C Patching Support added            by <Damian Coldbird> :
 *     :     > Y-Pos Type-A Patching Support added            by <Damian Coldbird> :
 *     :     > Y-Pos Type-C Patching Support added            by <Damian Coldbird> :
 *     :     > Screenmode Type-A Patching Support added       by <Damian Coldbird> :
 *     :     > Screenmode Type-C PAtching Support added       by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.11                                                             :
 *     :     > Y-Pos Values are now variable...               by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Static Gamecode Ammount -> Used for PAL Region Verification
#define N_GAME_CODES	4

//Region-Codes -> Used for PAL Region Verification
char *gamecodes[N_GAME_CODES] =
{
	"SLES",
	"SCES",
	"SCED",
	"SLED",
};

//Modes
int yfix=0;
int sfix=0;
unsigned short yval[2]={0x03,0x00};

/* FixVMode Void      #####################################
 * ----------------   | Return Values                     |
 * Fixes VMode and    |    None                           |
 * YFix / Screenfix.  #####################################
 */
void FixVMode(char * infile, char * outfile)
{
   //Try to open Inputfile
   FILE * f1 = fopen(infile,"rb");

   //Close Tool if File can't be opened!
   if(!f1)
   {
      fprintf(stderr,"<%s> -> Failed to Open Input PSX Game ISO!\n",infile);
      exit(-1);
   }

   //Try to open Outputfile
   FILE * f2 = fopen(outfile,"wb");

   //Close Tool if File can't be opened!
   if(!f2)
   {
      fprintf(stderr,"<%s> -> Failed to Open Output PSX Game ISO!\n",outfile);
      exit(-1);
   }

   //typo
   int typo=0;

   //fixes
   int countdown=0;

   //Contains the Ammount of Successfully Read Bytes via fread!
   int readbyte=0;

   //Buffer for Struct
   unsigned char buffer[4];

   while((readbyte=fread(buffer,1,sizeof(buffer),f1))==sizeof(buffer))
   {
      //Type
      typo=0;

      //If Countdown is set
      if(countdown)
      {
         countdown--;
      }

      //Do a Pre-Check on the Buffer
      if((buffer[1]<<8)+buffer[0]>1)
      {
         //Type A-Mask
         if(buffer[0]==0x0A &&
            buffer[1]==0x00 &&
            buffer[2]==0x24 &&
            buffer[3]==0x86)
         {
            typo=1;
            if(yfix||sfix) countdown=10;
            fprintf(stdout,"<%s> -> Patched Type-A VMode !\n",infile);
         }

         //Type B-Mask (Missing)

         //Type C-Mask
         if(buffer[0]==0x0A &&
            buffer[1]==0x00 &&
            buffer[2]==0x04 &&
            buffer[3]==0x86)
         {
            typo=3;
            if(yfix||sfix) countdown=10;
            fprintf(stdout,"<%s> -> Patched Type-C VMode !\n",infile);
         }
   
         //Type D-Mask (Missing)
      }

      //Apply Additional Fixes
      if(((countdown==5)||(countdown==4))&&(buffer[2]>=0x90)&&(buffer[2]<=0x93)&&(buffer[3]==0x24))
      {
         if(yfix)
         {
               if(countdown==5)
               {
                  (*(short*)(&buffer))=yval[0];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 1 !\n",infile);
               }
               if(countdown==4)
               {
                  (*(short*)(&buffer))=yval[1];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 2 !\n",infile);
               }
         }
         if(sfix)
         {
            if(buffer[2]>0x90)
            {
               buffer[2]--;
               fprintf(stdout,"<%s> -> Applied Screen-Fix !\n",infile);
            }
         }
         fwrite(buffer,1,4,f2);
      }
      //Apply Normal Fixes
      else
      {
         //Normal Copy... :)
         if(!typo)
         {
            fwrite(buffer,1,1,f2);
            fseek(f1,(1-sizeof(buffer)),SEEK_CUR);
         }

         //Exchange for Type-A / Type-C Fix
         if((typo==1)||(typo==3))
         {
            buffer[0]-=2;
            buffer[3]-=2;
            fwrite(buffer,1,4,f2);
         }

         //Exchange for Type-B Fix (Missing)
         if(typo==2)
         {

         }

         //Exchange for Type-D Fix (Missing)
         if(typo==4)
         {

         }
      }
   }

   //This Shouldnt happen... but better safe than sorry...
   if(readbyte)
   {
      fwrite(buffer,1,readbyte,f2);
   }

   //Close the Files... Work is over...
   fclose(f1);
   fclose(f2);
}

/* GetGID Function    #####################################
 * ----------------   | Return Values                     |
 * Filters out GID    |    0 = Failure  / char* = Success |
 * of the ISO.        #####################################
 */
char * GetGID(char * filename, char * output)
{
   FILE * file = fopen(filename, "rb");
   if(!file)
   {
      fprintf(stderr,"Couldn't Open PSX Game [%s] for GameID Scan!\n",filename);
      exit(-1);
   }

   int i;
   int x;

   char buffer[13];

	while ((x = fread(buffer, 1, 13, file)) == 13)
	{
   	for (i = 0; i < N_GAME_CODES; i++)
   	{
   		if ((strncmp(buffer, gamecodes[i], 4) == 0)&&
             (buffer[4]=='_')&&(buffer[8]=='.')&&(buffer[11]==';')&&(buffer[12]=='1'))
   			break;
   	}
      if(i!=N_GAME_CODES)
      {
         output[0]=gamecodes[i][0];
         output[1]=gamecodes[i][1];
         output[2]=gamecodes[i][2];
         output[3]=gamecodes[i][3];
         output[4]=buffer[5];
         output[5]=buffer[6];
         output[6]=buffer[7];
         output[7]=buffer[9];
         output[8]=buffer[10];
         output[9]='\0';
         break;
      }
      fseek(file,-12L,SEEK_CUR);
   }

   fclose(file);

   if(x!=13) return (char*)(0);
   else return output;
}

/* Roland Function      #####################################
 * -------------------  | Return Values                     |
 * Function used to     |    0 = Failure  /  1 = Success    |
 * check for IO Rights  #####################################
 */
int Roland(char * filename, char * mode)
{
   //File Pointer
   FILE * fp;

   //Check Read-Access
   if(!strcmp(mode,"rb"))
   {
      fp = fopen(filename,mode);
      if(fp)
      {
         fclose(fp);
         return 1;
      }
   }

   //Check Write Access
   else if(!strcmp(mode,"wb"))
   {
      fp = fopen(filename,mode);
      if(fp)
      {
         fclose(fp);
         remove(filename);
         return 1;
      }
   }

   //No Access Rights available!
   return 0;
}

/* GetFS Function     #####################################
 * ----------------   | Return Values                     |
 * Used to get        |    -1 = Failure  /  Success >= 0  |
 * Filesize.          #####################################
 */
int GetFS(char * filename)
{
   //File Pointer + Open File
   FILE * fp = fopen(filename,"rb");

   //If it can't be opened - Return EOF
   if(!fp) return 0;

   //Get Filesize
   int size;
   fseek(fp,0L,SEEK_END);
   size = ftell(fp);

   //Close File
   fclose(fp);

   //Return Filesize
   return size;
}

/* LetterVal          #####################################
 * ----------------   | Return Values                     |
 * Used to convert    |    0-15    (-1 on Failure)        |
 * char to hex.       #####################################
 */
int LetterVal(char c)
{
   switch(c)
   {
      case '0':
         return 0;
      case '1':
         return 1;
      case '2':
         return 2;
      case '3':
         return 3;
      case '4':
         return 4;
      case '5':
         return 5;
      case '6':
         return 6;
      case '7':
         return 7;
      case '8':
         return 8;
      case '9':
         return 9;
      case 'A':
      case 'a':
         return 10;
      case 'B':
      case 'b':
         return 11;
      case 'C':
      case 'c':
         return 12;
      case 'D':
      case 'd':
         return 13;
      case 'E':
      case 'e':
         return 14;
      case 'F':
      case 'f':
         return 15;
   }
   return -1;
}

/* GetYVal Function   #####################################
 * ----------------   | Return Values                     |
 * Used to get        |    0 = Failure  /  1 = Success    |
 * Y-Values.          #####################################
 */
int GetYVal(char * str, int offset)
{
   unsigned short result=0;

   if((strlen(str))!=4)
      return 0;

   if((str[0]!='0')||(str[1]!='x'))
      return 0;

   if(((str[2]>='0')&&(str[2]<='9'))||((str[2]>='A')&&(str[2]<='F'))||((str[2]>='a')&&(str[2]<='z')))
      result+=(0x10*(LetterVal(str[2])));
   else
      return 0;

   if(((str[3]>='0')&&(str[3]<='9'))||((str[3]>='A')&&(str[3]<='F'))||((str[3]>='a')&&(str[3]<='z')))
      result+=LetterVal(str[3]);
   else
      return 0;

   yval[offset]=result;

   return 1;
}

/* main Function      #####################################
 * ----------------   | Return Values                     |
 * Main Procedure     |    !=0 = Failure  /  0 = Success  |
 * of Paladin.        #####################################
 */
int main(int argc, char* argv[])
{
   //Print ASCII Graphixxx
   fprintf(stdout,"\n");
   fprintf(stdout," 8888888b.     d8888 888                    888 d8b\n");
   fprintf(stdout," 888   Y88b   d88888 888                    888 Y8P\n");
   fprintf(stdout," 888    888  d88P888 888                    888\n");
   fprintf(stdout," 888   d88P d88P 888 888       8888b.   .d88888 888 88888b.\n");
   fprintf(stdout," 8888888P\" d88P  888 888          \"88b d88\" 888 888 888 \"88b\n");
   fprintf(stdout," 888      d88P   888 888      .d888888 888  888 888 888  888\n");
   fprintf(stdout," 888     d8888888888 888      888  888 Y88b 888 888 888  888\n");
   fprintf(stdout," 888    d88P     888 88888888 \"Y888888  \"Y88888 888 888  888\n");
   fprintf(stdout,"\n");

   //Check Argument Count
   if((argc<3)||(argc>7))
   {
      fprintf(stdout,"%s <PAL> <NTSC> [Y-Fix] [Screen-Fix] [Y-Value 1] [Y-Value 2]\n",argv[0]);
      fprintf(stdout,"  ex. %s MyPAL.iso OutputNTSC.iso Yes Yes 0x03 0x00\n",argv[0]);
      fprintf(stdout,"\n");
      fprintf(stdout,"  <PAL>        : Input PAL ISO File\n");
      fprintf(stdout,"  <NTSC>       : Output NTSC ISO File\n");
      fprintf(stdout,"  [Y-Fix]      : Yes / No -> Automatically Resets Pos to 3/0\n");
      fprintf(stdout,"  [Screen-Fix] : Yes / No -> Decreases Screen Value by 1\n");
      fprintf(stdout,"  [Y-Value 1]  : Its the YFix 1 Value - If uninitialized Paladin will use 0x03\n");
      fprintf(stdout,"  [Y-Value 2]  : Its the YFix 2 Value - If unitinialized Paladin will use 0x00\n");
      fprintf(stdout,"\n");
      return 0;
   }

   //Check for File-Access Rights [Roland was one of the most famous Paladins in the Middle Age]
   if(!(Roland(argv[1],"rb")))
   {
      fprintf(stderr,"<%s> -> File couldn't be opened.\n",argv[1]);
      return -1;
   }

   //Check Filesize
   if((GetFS(argv[1]))==0)
   {
      fprintf(stderr,"<%s> -> 0-Byte-Error / Access Violation.\n",argv[1]);
      return -2;
   }

   //Check if Game is European
   char gameid[10];
   if(!(GetGID(argv[1],gameid)))
   {
      fprintf(stderr,"<%s> -> File ain't a European PSX-Game!\n",argv[1]);
      return -3;
   }

   fprintf(stdout,"<%s> -> GameID [%s] verified as PAL ID!\n",argv[1],gameid);

   //Set Modes
   if(!(strcmp(argv[3],"Yes"))) yfix=1;
   if(!(strcmp(argv[4],"Yes"))) sfix=1;

   //Set YVals
   if(argc>=6)
   {
      if((GetYVal(argv[5],0))==-1)
      {
         fprintf(stdout,"<%s> -> Hex Value given for Y-Value 1 is invalid! Standard Value will be used!\n",argv[1]);
      }
   }
   if(argc==7)
   {
      if((GetYVal(argv[6],1))==-1)
      {
         fprintf(stdout,"<%s> -> Hex Value given for Y-Value 2 is invalid! Standard Value will be used!\n",argv[1]);
      }
   }

   //Fix the Game
   FixVMode(argv[1],argv[2]);

   //ByeBye!
   return 0;
}
