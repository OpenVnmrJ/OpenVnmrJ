BEGIN{}

/pulsesequence\(\)/{psf=1;
   printf "/* SPARSE ONE*/                                                                           \n";
   printf "#define MAXFIDS 400000\n";
   printf "static int sel[MAXFIDS][%d];                                                             \n",NDIM-1 ;
if(NDIM == 4){printf("static double  d4const, d3const, d2const;                            \n"); }
if(NDIM == 3){printf("static double  d3const, d2const;                                     \n"); }
if(NDIM == 2){printf("static double  d2const;                                              \n"); }
   printf "/* SPARSE ONE*/                                                                           \n";
   printf "\n\n                                                                                     \n";
}

/getstr\(/{if(psf==1) { psf=2;
   printf "  /* SPARSE TWO */								    	   \n";
   printf "  { char  sparse_dir[MAXSTR],sparse_file[MAXSTR],sparse[MAXSTR];				   \n";
   printf "    getstr(\"SPARSE\",sparse);						    \n";
   printf "    if(sparse[0]=='y'){      \n";
   printf "    if(ix==1)       								   \n";
   printf "    {   int i;								   \n";
   printf "        FILE     *fsparse_table;                                                              \n";
   printf "        getstr(\"sparse_file\",sparse_file);						    \n";
   printf "        getstr(\"sparse_dir\",sparse_dir);						    \n";
   printf "        strcat(sparse_dir,\"/\");					            \n";
   printf "        strcat(sparse_dir,sparse_file);					            \n";
   printf "        strcat(sparse_dir,\".hdr_3\");					            \n";
   printf "        fsparse_table=fopen(sparse_dir,\"r\");		   		          \n";
   printf "        if(NULL==fsparse_table){printf(\"File %%s not found\\n\",sparse_dir); psg_abort(1);}                       \n";
if(NDIM == 4){printf("  d4const=d4; d3const=d3; d2const=d2;                                                    \n"); }
if(NDIM == 3){printf("              d3const=d3; d2const=d2;                                                    \n"); }
if(NDIM == 2){printf("                          d2const=d2;                                                    \n"); }

if(NDIM == 4){printf(" for(i=0;i<ni*getval(\"ni2\")*getval(\"ni3\")*3;i++)                                              \n"); }
if(NDIM == 3){printf(" for(i=0;i<ni*getval(\"ni2\")*2;i++)                                                  \n"); }
if(NDIM == 2){printf(" for(i=0;i<ni;i++)                                                   	   \n"); }
   printf "            {   if(i>=MAXFIDS)                                \n";
   printf "                { printf(\"Too many FIDs \\n\"); psg_abort(1);}                     \n";
   printf "                if(1!=fscanf(fsparse_table,\"%%d\",(int*)sel+i))                                \n";
   printf "                { printf(\"Too few lines in %%s\\n\",sparse_file); psg_abort(1);}                     \n";
   printf "            }          									   \n";
   printf "            fclose(fsparse_table);								   \n";
   printf "            printf(\"Sampling table was read from %%s\\n\",sparse_dir);								   \n";
   printf "    }										   \n";
if(NDIM == 4) { printf("d2=d2const+sel[(int)(ix-1)/8][0]/sw1; d3=d3const+sel[(int)(ix-1)/8][1]/sw2; d4=d4const+sel[(int)(ix-1)/8][2]/sw3;    \n"); 
}
if(NDIM == 3) { printf("d2=d2const+sel[(int)(ix-1)/4][0]/sw1; d3=d3const+sel[(int)(ix-1)/4][1]/sw2;					   \n"); 
 printf "if(ix<=16)printf(\"%%d %%d\\n\",sel[(int)(ix-1)/4][0],sel[(int)(ix-1)/4][1]);\n"; 
}
if(NDIM == 2) { printf("d2=d2const+sel[(int)(ix-1)/2][0]/sw1;						  				 \n"); }
   printf "  }								   \n";
   printf " /* SPARSE TWO */ }\n\n						   \n";
}
}
{print $0;}
END{}

