#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNK 0xFF
#define N_GAME_CODES	4

char *gamecodes[N_GAME_CODES] =
{
	"SLES",
	"SCES",
	"SCED",
	"SLED",
};

unsigned char vmodemask[4] = {UNK,UNK,0x24,0x86};
unsigned char yposmask[16] = {UNK,UNK,0x24,0x86,
                              UNK,UNK,UNK,UNK,UNK,UNK,0x90,0x24,UNK,UNK,0x90,0x24};

int GetMask(char * filename)
{
   FILE * f1 = fopen(filename,"r");

   if(!f1)
   {
      fprintf(stderr,"Failed to open [%s] for Mask-Scan!\n",filename);
      exit(-1);
   }

   int okay=0;
   int readbyte=0;
   int i=0;
   unsigned char buffer[16];

   while((readbyte=fread(buffer,1,16,f1))==sizeof(buffer))
   {
      //Comparison Switch
      okay=1;

      //Do a Pre-Check on the Buffer
      if((buffer[0]!=0x00)||(buffer[1]!=0x00))
      {
         //Compare Buffer with VMode Searchmask
         for(i=0;i<sizeof(yposmask);i++)
         {
            //If VMode Searchmask doesn't allow Wildcard then...
            if(yposmask[i]!=UNK)
            {
               //If Buffer-Content != VMode Searchmask -> Comparison Unsuccessful!
               if(yposmask[i]==0x90)
               {
                  if(!((buffer[i]==yposmask[i])||(buffer[i]==yposmask[i]+1)||(buffer[i]==yposmask[i]+2)))
                  {
                     okay=0;
                     break;
                  }
               }
               else
               {
                  if(buffer[i]!=yposmask[i])
                  {
                     okay=0;
                     break;
                  }
               }
            }
         }
      }
      //Pre-Check Failed! 0x0000 or 0x0001 Detected - This Value CAN NEVER be the VMode in a PAL Game!
      else
      {
         okay=0;
      }

      //If the Searchmask was a Success - Write Fixed Value!
      if(okay)
      {
         vmodemask[0]=buffer[0];
         vmodemask[1]=buffer[1];
         vmodemask[2]=buffer[2];
         vmodemask[3]=buffer[3];
         fprintf(stdout,"[%x] - VMode Struct Found!\n",*(unsigned int*)(&vmodemask));
         return 1;
      }
      else
      {
         fseek(f1,-15L,SEEK_CUR);
      }
   }

   fclose(f1);

   return 0;
}

char * GetGID(char * filename, char * output)
{
   FILE * file = fopen(filename, "r");
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

int main(int argc, char * argv[])
{
   //Check Argument Count
   if(argc!=3)
   {
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
      fprintf(stdout,"%s <PAL> <NTSC> [Y]\n",argv[0]);
      fprintf(stdout,"  ex. %s MyPAL.iso OutputNTSC.iso\n",argv[0]);
      fprintf(stdout,"\n");
      fprintf(stdout,"  <PAL>     : Input PAL ISO File\n");
      fprintf(stdout,"  <NTSC>    : Output NTSC ISO File\n");
      fprintf(stdout,"\n");
      return 0;
   }

   //Open Inputfile
   FILE * f1 = fopen(argv[1],"r");
   if(!f1)
   {
      fprintf(stderr,"%s couldn't be opened.",argv[1]);
      return -1;
   }

   //Get Filesize
   int s1;
   fseek(f1,0L,SEEK_END);
   s1 = ftell(f1);

   if(s1==0)
   {
      fprintf(stderr,"%s is 0-Byte long... Terminating...",argv[1]);
      return -2;
   }

   fclose(f1);

   //Check if Game is European
   char gameid[9];
   if(!(GetGID(argv[1],gameid)))
   {
      fprintf(stderr,"[%s] ain't a European PSX-Game!\n",argv[1]);
      return -3;
   }

   //Check for Y-Pos Structure
   if(argc==3)
   {
      if(!(GetMask(argv[1])))
      {
         fprintf(stdout,"[%s] doesn't contain a Y-Pos Structure!\n",argv[1]);
         fprintf(stdout,"Do you intend to Patch using the Standard VMode Mask?\n");
         fprintf(stdout,"Keep in Mind that Patching using the Standard Mask can corrupt your Game...\n");
         fprintf(stdout,"[Space Key -> YES]  [Any Other Printable Key -> NO]\n");
         if((fgetc(stdin))==' ')
         {
            fprintf(stdout," Scanning with Standard Mask - Prepare for Broken Games... :P\n");
         }
         else
         {
            fprintf(stdout," Stopped Process - No Y-Pos Structure containing VMode Mask found!\n");
            return 0;
         }
      }
   }

   //Reopen Inputfile
   f1 = fopen(argv[1],"r");

   //Open Outputfile
   FILE * f2 = fopen(argv[2],"w");
   if(!f2)
   {
      fprintf(stderr,"%s couldn't be opened.",argv[2]);
      return -1;
   }

   //Fix the Shiz...
   int readbyte=0;
   int okay=0;
   int i=0;
   unsigned char buffer[4];
   unsigned char fix[4];

   while((readbyte=fread(buffer,1,4,f1))==sizeof(buffer))
   {
      //Comparison Switch
      okay=1;

      //Do a Pre-Check on the Buffer
      if((buffer[0]!=0x00)||(buffer[1]!=0x00))
      {
         //Compare Buffer with VMode Searchmask
         for(i=0;i<sizeof(vmodemask);i++)
         {
            //If VMode Searchmask doesn't allow Wildcard then...
            if(vmodemask[i]!=UNK)
            {
               //If Buffer-Content != VMode Searchmask -> Comparison Unsuccessful!
               if(buffer[i]!=vmodemask[i])
               {
                  okay=0;
                  break;
               }
            }
         }
      }
      //Pre-Check Failed! 0x0000 or 0x0001 Detected - This Value CAN NEVER be the VMode in a PAL Game!
      else
      {
         okay=0;
      }

      //If the Searchmask was a Success - Write Fixed Value!
      if(okay)
      {
         fix[0]=buffer[0]-2;
         fix[1]=buffer[1];
         fix[2]=buffer[2];
         fix[3]=buffer[3]-2;
         fwrite(fix,1,4,f2);
         fprintf(stdout,"  -> VMode Struct Replaced!\n");
      }
      else
      {
         fwrite(buffer,1,1,f2);
         fseek(f1,-3L,SEEK_CUR);
      }
   }
   if(readbyte)
   {
      fwrite(buffer,1,readbyte,f2);
   }

   //Close Files
   fclose(f1);
   fclose(f2);

   //ByeBye
   return 0;
}
