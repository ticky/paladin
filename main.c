/*
 * Paladin - Enhanced PAL to NTSC Converter for True PSX NTSC Conversion
 * Copyright (C) 2007  Damian Coldbird <vanburace@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *    8888888b.     d8888 888                    888 d8b          R
 *    888   Y88b   d88888 888                    888 Y8P          .E
 *    888    888  d88P888 888                    888               .V
 *    888   d88P d88P 888 888       8888b.   .d88888 888 88888b.    .I      [1.3]
 *    8888888P" d88P  888 888          "88b d88" 888 888 888 "88b    .S
 *    888      d88P   888 888      .d888888 888  888 888 888  888     .I
 *    888     d8888888888 888      888  888 Y88b 888 888 888  888      .O
 *    888    d88P     888 88888888 "Y888888  "Y88888 888 888  888       .N
 *
 *    Enhanced PAL to NTSC Converter for True PSX NTSC Conversion
 *
 *    Changelog :...................................................................
 *     : REVISION 1.0                                                              :
 *     :     > Initial Release of PALadin - Welcome on Earth! by <Damian Coldbird> :
 *     :     > VMode Type-A Patching Support added            by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.1                                                              :
 *     :     > VMode Type-C Patching Support added            by <Damian Coldbird> :
 *     :     > Y-Pos Type-A Patching Support added            by <Damian Coldbird> :
 *     :     > Y-Pos Type-C Patching Support added            by <Damian Coldbird> :
 *     :     > Screenmode Type-A Patching Support added       by <Damian Coldbird> :
 *     :     > Screenmode Type-C PAtching Support added       by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.11                                                             :
 *     :     > Y-Pos Values are now variable...               by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.2                                                              :
 *     :     > Now Released under the GNU-GPL                 by <Damian Coldbird> :
 *     :     > Now using Rolling Buffer for Speed Increase    by <flatwhatson>     :
 *     :     > Improved Handling of Arguments                 by <Damian Coldbird> :
 *     :     > VMode outsourced to optional Argument          by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 *     : REVISION 1.3                                                              :
 *     :     > Rolling Buffer extended                        by <flatwhatson>     :
 *     :     > Enhanced Speed & Less Corruption due to        by <Damian Coldbird> :
 *     :       PS-X EXE LBA Limitation.                                            :
 *     :     > Expansion of YPos Limit to 'Real' Values       by <Damian Coldbird> :
 *     :       (Negative YPos Values now supported!)                               :
 *     :     > Allow for NTSC / SECAM Games to be patched     by <Damian Coldbird> :
 *     :.::..::..::..::..::..::..::..::..::..::..::..::..::..::.::..::..::..::..::.:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Static Gamecode Ammount -> Used for PAL Region Verification
#define N_PAL_CODES	4
#define N_GAME_CODES	12

char gameid[10]={0};

//Region-Codes -> Used for PAL Region Verification
char *palcodes[N_PAL_CODES] =
{
	"SLES",
	"SCES",
	"SCED",
	"SLED",
};

char *gamecodes[N_GAME_CODES] =
{
	"SCUS",
	"SLUS",
	"SLES",
	"SCES",
	"SCED",
	"SLPS",
	"SLPM",
	"SCPS",
	"SLED",
	"SLPS",
	"SIPS",
	"ESPM"
};

//Executeable Information
typedef struct execinfo_s
{
   char execname[64];
   int execlba;
   int execoffset;
   int execsize;
} execinfo_t;

int execcount=0;
execinfo_t isoinfo[10];

//Modes
int vfix=1;
int yfix=0;
int sfix=0;
unsigned char sbase=0x90;
short yval[2]={0x03,0x00};

//Rolling Buffer Stuff <flatwhatsons' area...>
#define BUFFSIZE 0x100000
unsigned char * read_buffer;
int read_end;
int read_offset;
int read_init = 0;
unsigned char * write_buffer;
int write_offset;
int write_init = 0;

/* buffread Function  #####################################
 * -----------------  | Return Values                     |
 * Reads file via a   |    Amount of Read Bytes           |
 * Rolling Buffer.    #####################################
 */
int buffread(unsigned char * buffer, int size, FILE * fp)
{
	int i;
	/* Initialise read buffer
	 * - allocate memory
	 * - fill with data from file
	 */
    if(!read_init)
    {
    	read_buffer=malloc(BUFFSIZE);
        read_end=fread(read_buffer,1,BUFFSIZE,fp);
        read_offset=0;
        read_init=1;
    }

   /* Manual flush read buffer
    * (call with size==0)
    * - clean up write buffer
    */
   if(!size)
   {
      free(read_buffer);
      read_init=0;
      return 0;
   }

    /* Roll read buffer
     * - not enough data to return 'size' unread bytes
     * - copy remaining unread to start of buffer
     * - fill remainder of buffer from file
     */
    if(read_offset+size>=BUFFSIZE)
    {
        memcpy(read_buffer,read_buffer+read_offset,BUFFSIZE-read_offset);
        read_end=fread(read_buffer+BUFFSIZE-read_offset,1,read_offset,fp)+BUFFSIZE-read_offset;
        read_offset=0;
    }
    /* Fill target buffer
     * - write unread bytes into 'buffer'
     */
    for(i=0;i<size;i++)
    {
    	buffer[i]=read_buffer[read_offset+i];
    }
    /* End of file
     * - clean up read buffer
     * - return number of unread bytes
     */
    if(read_offset+size>read_end)
    {
    	free(read_buffer);
    	read_init=0;
        return read_end-read_offset;
    }
    /* Successful buffread
     * - return number of unread bytes copied to 'buffer'
     */
    else
    {
    	read_offset+=size;
        return size;
    }
}

/* buffwrite Function #####################################
 * ------------------ | Return Values                     |
 * Writes file via a  |    Amount of Written Bytes        |
 * Rolling Buffer.    #####################################
 */
int buffwrite(unsigned char * buffer, int size, FILE * fp)
{
	int i;
	/* Initialise write buffer
	 * - allocate memory
	 */
	if(!write_init)
	{
		write_buffer=malloc(BUFFSIZE);
		write_init=1;
	}
	/* Manual flush write buffer
	 * (call with size==0)
	 * - write unwritten bytes from write buffer
	 * - clean up write buffer
	 */
	if(!size)
	{
		fwrite(write_buffer,1,write_offset,fp);
		free(write_buffer);
		write_init=0;
		return 0;
	}
	/* Auto flush write buffer
	 * (no more room in buffer)
	 * - write unwritten bytes from write buffer
	 * - reset write buffer
	 */
	if(write_offset+size>=BUFFSIZE)
	{
		fwrite(write_buffer,1,write_offset,fp);
		write_offset=0;
	}
	/* Fill write buffer
	 * - copy into write buffer from source buffer
	 */
	for(i=0;i<size;i++)
	{
		write_buffer[write_offset+i]=buffer[i];
	}
	/* Successful write
	 * - return number of bytes stored in write buffer
	 */
	write_offset+=size;
	return size;
}

/* bufftell Function  #####################################
 * ------------------ | Return Values                     |
 * Rolling Buffer     |    Amount of Written Bytes        |
 * ftell Equivalent.  #####################################
 */
int bufftell(FILE * fp)
{
   return ftell(fp)-BUFFSIZE+read_offset;
}

/* FixVMode Void      #####################################
 * -----------------  | Return Values                     |
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

   //Contains the Ammount of Successfully Read Bytes via fread!
   int readbyte=0;

   //Buffer for Struct
   unsigned char buffer[16];

   //Are we inside a Executeable?
   int in_exe=0;

   //Needed Variable
   int i;

   //Executeable Information
   fprintf(stdout,"\n");
   for(i=0;i<execcount;i++)
   {
      fprintf(stdout,"---> Executeable #%i Information <---\n",i);
      fprintf(stdout,"Name   -> [%s]\n",isoinfo[i].execname);
      fprintf(stdout,"LBA    -> [%i]\n",isoinfo[i].execlba);
      fprintf(stdout,"Offset -> [%i]\n",isoinfo[i].execoffset);
      fprintf(stdout,"Size   -> [%i]\n",isoinfo[i].execsize);
      fprintf(stdout,"\n");
   }

   //while((readbyte=fread(buffer,1,sizeof(buffer),f1))==sizeof(buffer))
   while((readbyte=buffread(buffer,sizeof(buffer),f1))==sizeof(buffer))
   {
      //Reset Executeable Check
      in_exe=0;

      //Type
      typo=0;

      //Check if we are currently inside a Executeable
      for(i=0;i<execcount;i++)
      {
         if(((bufftell(f1))>=isoinfo[i].execoffset)&&((bufftell(f1))<=(isoinfo[i].execoffset+isoinfo[i].execsize)))
         {
            in_exe=1;
            break;
         }
      }

      //If we are in a Executeable... Well... Prepare for Patching!
      if(in_exe)
      {
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
            }

            //Type B-Mask
            if(buffer[0]==0x08 &&
               buffer[3]==0x91 &&
               buffer[4]==0x24)
            {
               typo=2;
            }

            //Type C-Mask
            if(buffer[0]==0x0A &&
               buffer[1]==0x00 &&
               buffer[2]==0x04 &&
               buffer[3]==0x86)
            {
               typo=3;
            }
         }
      }

      //Type-A Struct
      if(typo==1)
      {
         //VMode Fix
         if(vfix)
         {
            buffer[0]-=2;
            buffer[3]-=2;
            fprintf(stdout,"<%s> -> Patched VMode [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
         }

         //YPos Fix
         if(yfix)
         {
            if(buffer[10]>=sbase &&
               buffer[10]<=0x94 &&
               buffer[11]==0x24 &&
               buffer[14]>=sbase &&
               buffer[14]<=0x94 &&
               buffer[15]==0x24)
            {
               if(yfix==2)
               {
               	(*(short*)(&buffer+8))+=yval[0];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 1 [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
                  (*(short*)(&buffer+12))+=yval[1];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 2 [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               }
               else
               {
                  (*(short*)(&buffer+8))=yval[0];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 1 [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
                  (*(short*)(&buffer+12))=yval[1];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 2 [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               }
            }
         }

         //Screenmode Fix
         if(sfix)
         {
            if(buffer[10]>=sbase &&
               buffer[10]<=0x94 &&
               buffer[11]==0x24 &&
               buffer[14]>=sbase &&
               buffer[14]<=0x94 &&
               buffer[15]==0x24)
            {
               buffer[10]--;
               fprintf(stdout,"<%s> -> Applied Screen-Fix [A] in Executeable [%s] !\n",infile,isoinfo[i].execname);
            }
         }
      }

      //Type-B Struct
      if(typo==2)
      {
         //YPos Fix
         if(yfix)
         {
         	if(yfix==2)
         	{
         	   (*(short*)(&buffer+1))+=yval[0];
               fprintf(stdout,"<%s> -> Applied Y-Fix 1 [B] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               (*(short*)(&buffer+5))+=yval[1];
               fprintf(stdout,"<%s> -> Applied Y-Fix 2 [B] in Executeable [%s] !\n",infile,isoinfo[i].execname);
         	}
         	else
         	{
               (*(short*)(&buffer+1))=yval[0];
               fprintf(stdout,"<%s> -> Applied Y-Fix 1 [B] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               (*(short*)(&buffer+5))=yval[1];
               fprintf(stdout,"<%s> -> Applied Y-Fix 2 [B] in Executeable [%s] !\n",infile,isoinfo[i].execname);
         	}
         }
      }

      //Type-C Struct
      if(typo==3)
      {
         //VMode Fix
         if(vfix)
         {
            buffer[0]-=2;
            buffer[3]-=2;
            fprintf(stdout,"<%s> -> Patched VMode [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
         }

         //YPos Fix
         if(yfix)
         {
            if(buffer[10]>=sbase &&
               buffer[10]<=0x94 &&
               buffer[11]==0x24 &&
               buffer[14]>=sbase &&
               buffer[14]<=0x94 &&
               buffer[15]==0x24)
            {
               if(yfix==2)
               {
               	(*(short*)(&buffer+8))+=yval[0];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 1 [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
                  (*(short*)(&buffer+12))+=yval[1];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 2 [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               }
               else
               {
                  (*(short*)(&buffer+8))=yval[0];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 1 [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
                  (*(short*)(&buffer+12))=yval[1];
                  fprintf(stdout,"<%s> -> Applied Y-Fix 2 [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
               }
            }
         }

         //Screenmode Fix
         if(sfix)
         {
            if(buffer[10]>=sbase &&
               buffer[10]<=0x94 &&
               buffer[11]==0x24 &&
               buffer[14]>=sbase &&
               buffer[14]<=0x94 &&
               buffer[15]==0x24)
            {
               buffer[10]--;
               fprintf(stdout,"<%s> -> Applied Screen-Fix [C] in Executeable [%s] !\n",infile,isoinfo[i].execname);
            }
         }
      }


      //Write Changes
      if(typo)
      {
         //Write Edited Struct
         buffwrite(buffer,sizeof(buffer),f2);
      }
      else
      {
         //Write One Byte and Revert
         buffwrite(buffer,1,f2);
         read_offset-=sizeof(buffer)-1;
      }
   }

   //This Shouldnt happen... but better safe than sorry...
   if(readbyte)
   {
      buffwrite(buffer,readbyte,f2);
   }
   
   //Flush read buffer
   buffread(buffer,0,f1);

   //Flush write buffer
   buffwrite(buffer,0,f2);

   //Close the Files... Work is over...
   fclose(f1);
   fclose(f2);
}

/* GetInfo Function   #####################################
 * ----------------   | Return Values                     |
 * Filters out Info   |    0 = Failure  / 1,2 = Success   |
 * of the ISO.        #####################################
 */
int GetInfo(char * filename)
{
   //Open and Check Filepointer
   FILE * file = fopen(filename, "rb");
   if(!file)
   {
      fprintf(stderr,"Couldn't Open PSX Game [%s] for GameInfo Scan!\n",filename);
      exit(-1);
   }

   //Needed Variable
   int i;
   int x;
   int skip=0;

   //Needed Buffer
   char buffer[64];

   //While the File ain't EOF
	while ((x = buffread((unsigned char*)buffer, 64, file)) == 64)
	{
      if(gameid[0]==0)
      {
         //Check if the ISO actually is a PSX Game
         for (i = 0; i < N_GAME_CODES; i++)
   	   {
   	      if ((strncmp(buffer+31, gamecodes[i], 4) == 0)&&
   	          (buffer[35]=='_')&&(buffer[39]=='.')&&(buffer[42]==';')&&(buffer[43]=='1')&&
                (buffer[0]==buffer[7])&&(buffer[1]==buffer[6])&&(buffer[2]==buffer[5])&&(buffer[3]==buffer[4])&&
                (buffer[8]==buffer[15])&&(buffer[9]==buffer[14])&&(buffer[10]==buffer[13])&&(buffer[11]==buffer[12]))
   	         break;
   	   }

         //If the ISO is a PSX Game -> Extract a ISO LBA Header
   	   if(i!=N_GAME_CODES)
   	   {
            //GameID Filtering
   	      gameid[0]=gamecodes[i][0];
   	      gameid[1]=gamecodes[i][1];
   	      gameid[2]=gamecodes[i][2];
   	      gameid[3]=gamecodes[i][3];
   	      gameid[4]=buffer[36];
   	      gameid[5]=buffer[37];
   	      gameid[6]=buffer[38];
   	      gameid[7]=buffer[40];
   	      gameid[8]=buffer[41];
   	      gameid[9]='\0';

            //Executeable Name Filtering
            strcpy(isoinfo[execcount].execname, buffer+31);

            //Executeable LBA Filtering
            isoinfo[execcount].execlba=*(unsigned int*)(buffer);

            //Executeable Offset Filtering
            isoinfo[execcount].execoffset=24+((*(unsigned int*)(buffer))*0x930);

            //Executeable Size Filtering
            isoinfo[execcount].execsize=*(unsigned int*)(buffer+8);

            //Increase Executeable Counter
            execcount++;

            //Skip Normal Check
            skip=1;
   	   }
      }

      if ((buffer[31]!=0)&&(!skip)&&
            (buffer[0]==buffer[7])&&(buffer[1]==buffer[6])&&(buffer[2]==buffer[5])&&(buffer[3]==buffer[4])&&
            (buffer[8]==buffer[15])&&(buffer[9]==buffer[14])&&(buffer[10]==buffer[13])&&(buffer[11]==buffer[12]))
      {
         //Backup old Position
         long pos=ftell(file);

         //Move to new Position
         fseek(file,24L+((*(unsigned int*)(buffer))*0x930),SEEK_SET);

         //Temporary Buffer
         char buf[9];

         //Read Information
         fread(buf,1,8,file);
         buf[8]='\0';

         //Check if its a PS-X EXE
         if(!strcmp(buf,"PS-X EXE"))
         {
            //Executeable Name Filtering
            strcpy(isoinfo[execcount].execname, buffer+31);

            //Executeable LBA Filtering
            isoinfo[execcount].execlba=*(unsigned int*)(buffer);

            //Executeable Offset Filtering
            isoinfo[execcount].execoffset=24+((*(unsigned int*)(buffer))*0x930);

            //Executeable Size Filtering
            isoinfo[execcount].execsize=*(unsigned int*)(buffer+8);

            //Increase Executeable Counter
            execcount++;
         }

         //Revert to old Position
         fseek(file,pos,SEEK_SET);
      }

      //Kill Skip
      skip=0;

      //Rollback the File
      read_offset-=63;

      //Are we already above 1MB?
      if((bufftell(file))>=(1024*1024))
      {
         break;
      }
   }

   //Flush Buffer
   buffread((unsigned char*)buffer,0,file);

   //Close File - Our Work here is done...
   fclose(file);

   //If the Game is a PAL Game
   for (i = 0; i < N_PAL_CODES; i++)
   {
      if ((strncmp(gameid, palcodes[i], 4) == 0))
         break;
   }

   //Game is a PAL Games
   if(i!=N_PAL_CODES)
   {
      return 2;
   }

   //ISO ain't a PSX Game
   if(x!=64) return 0;
   //ISO is a NON-PAL Game
   else return 1;
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
   short result=0;
   int mode=0;
   int noobval;

   if((strlen(str))!=4&&(strlen(str))!=5)
      return 0;

   if(str[0]=='-')
      mode=1;

   if(!(noobval=atoi(str)))
   {
      if((str[0+mode]!='0')||(str[1+mode]!='x'))
         return 0;

      if(((str[2+mode]>='0')&&(str[2+mode]<='9'))||((str[2+mode]>='A')&&(str[2+mode]<='F'))||((str[2+mode]>='a')&&(str[2+mode]<='z')))
         result+=(0x10*(LetterVal(str[2+mode])));
      else
         return 0;

      if(((str[3+mode]>='0')&&(str[3+mode]<='9'))||((str[3+mode]>='A')&&(str[3+mode]<='F'))||((str[3+mode]>='a')&&(str[3+mode]<='z')))
         result+=LetterVal(str[3+mode]);
      else
         return 0;
      
      if(mode)
         result*=-1;
   }
   else
   {
      result=noobval;
      if(yfix)
         yfix=2;
   }

   yval[offset]=result;

   return 1;
}

/* main Function      #####################################
 * ----------------   | Return Values                     |
 * Main Procedure     |    !=0 = Failure  /  0 = Success  |
 * of Paladin.        #####################################
 */
int main(int argc, char * argv[])
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
   if((argc<3)||(argc>8))
   {
      fprintf(stdout,"%s <PAL> <NTSC> [VMode-Fix] [Y-Fix] [Screen-Fix] [Y-Value 1] [Y-Value 2]\n",argv[0]);
      fprintf(stdout,"  ex. %s MyPAL.iso OutputNTSC.iso Yes Yes Force 0x03 0x00\n",argv[0]);
      fprintf(stdout,"\n");
      fprintf(stdout,"  <PAL>        : Input PAL ISO File\n");
      fprintf(stdout,"  <NTSC>       : Output NTSC ISO File\n");
      fprintf(stdout,"  [VMode-Fix]  : Yes / No -> Standard Setting [Yes]\n");
      fprintf(stdout,"  [Y-Fix]      : Yes / No -> Automatically Resets Pos to Custom Value\n");
      fprintf(stdout,"  [Screen-Fix] : Yes / No / Force -> Decreases Screen Value by 1\n");
      fprintf(stdout,"                 Force -> Forces a Screen-Fix, even on Values <= 0x90\n");
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

   //Get Game Information
   int region;
   if((region=GetInfo(argv[1]))==1)
   {
      fprintf(stdout,"<%s> -> GameID [%s] ain't a PAL ID!\n",argv[1],gameid);
      fprintf(stdout,"<%s> -> Patching most probably will corrupt your ISO... Beware...\n",argv[1]);
   }
   else if(region==2)
   {
      fprintf(stdout,"<%s> -> GameID [%s] verified as PAL ID!\n",argv[1],gameid);
   }
   else
   {
      fprintf(stderr,"<%s> -> ISO ain't a PSX Game!\n",argv[1]);
      return -3;
   }

   //Check if there are any Patchable Executeables inside...
   if(!execcount)
   {
      fprintf(stderr,"<%s> -> Couldn't find a PSX Executeable inside the ISO!\n",argv[1]);
      return -4;
   }

   //Set Modes
   int j;

   if(argc>=4)
   {
      for(j=0;j<strlen(argv[3]);j++)
         argv[3][j]=toupper(argv[3][j]);
      if(!(strcmp(argv[3],"NO"))) vfix=0;
   }
   if(argc>=5)
   {
      for(j=0;j<strlen(argv[4]);j++)
         argv[4][j]=toupper(argv[4][j]);
      if(!(strcmp(argv[4],"YES"))) yfix=1;
   }
   if(argc>=6)
   {
      for(j=0;j<strlen(argv[5]);j++)
         argv[5][j]=toupper(argv[5][j]);
      if(!(strcmp(argv[5],"YES"))) sfix=1;
      if(!(strcmp(argv[5],"FORCE")))
      {
         sbase=0x00;
         sfix=1;
      }
   }

   //Set YVals
   if(argc>=7)
   {
      if(!(GetYVal(argv[6],0)))
      {
         fprintf(stdout,"<%s> -> Hex Value given for Y-Value 1 is invalid! Standard Value will be used!\n",argv[1]);
      }
      else if(yfix==2)
      {
         yval[1]=yval[0];
      }
      else
      {
         yval[1]=yval[0]-3;
      }
   }
   if(argc==8)
   {
      if(!(GetYVal(argv[7],1)))
      {
         fprintf(stdout,"<%s> -> Hex Value given for Y-Value 2 is invalid! Standard Value will be used!\n",argv[1]);
      }
   }

   //Fix the Game
   FixVMode(argv[1],argv[2]);

   //ByeBye!
   return 0;
}
