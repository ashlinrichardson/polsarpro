/********************************************************************
PolSARpro v5.0 is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License as 
published by the Free Software Foundation; either version 2 (1991) of
the License, or any later version. This program is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. 

See the GNU General Public License (Version 2, 1991) for more details

*********************************************************************

File  : dey_bhattacharya_frery_lopez_rao_4components_decomposition_FP.c
Project  : ESA_POLSARPRO
Authors  : Eric POTTIER
Version  : 1.0
Creation : 08/2021
Update  :
*--------------------------------------------------------------------
INSTITUT D'ELECTRONIQUE et de TELECOMMUNICATIONS de RENNES (I.E.T.R)
UMR CNRS 6164

Waves and Signal department
SHINE Team 

UNIVERSITY OF RENNES I
B�t. 11D - Campus de Beaulieu
263 Avenue G�n�ral Leclerc
35042 RENNES Cedex
Tel :(+33) 2 23 23 57 63
Fax :(+33) 2 23 23 69 63
e-mail: eric.pottier@univ-rennes1.fr

*--------------------------------------------------------------------

Description :  S Dey & al. - 4 Components Decomposition

A Model-free Four Component Scattering Power Decomposition for 
Polarimetric SAR Data.
Dey S, Bhattacharya A, Frery A, L�pez-Mart�nez C
IEEE JSTARS, vol 14, 2021

Target Characterization and Scattering Power Decomposition for Full
and Compact Polarimetric SAR Data
Dey S, Bhattacharya A, Ratha D, Mandal D, Frery A

*--------------------------------------------------------------------

Adapted from c routine "MF4CF.c"
written by : Ash Richardson 
Senior Data Scientist, BC Wildfire Service, Canada 
2021/04/21 

********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "omp.h"

#ifdef _WIN32
#include <dos.h>
#include <conio.h>
#endif

/* ROUTINES DECLARATION */
#include "../lib/PolSARproLib.h"

/********************************************************************
*********************************************************************
*
*            -- Function : Main
*
*********************************************************************
********************************************************************/
int main(int argc, char *argv[])
{

#define NPolType 3
/* LOCAL VARIABLES */
  FILE *out_odd, *out_dbl, *out_vol, *out_hlx;
  FILE *out_theta, *out_tau;
  int Config;
  char *PolTypeConf[NPolType] = {"S2", "C3", "T3"};
  char file_name[FilePathLength];
  
/* Internal variables */
  int ii, lig, col;
  int ligDone = 0;

  float Span, SpanMin, SpanMax;
  float Ps, Pd, Pv, Pc;
  float theta, tau;
  double det_re, det_im, trace, trace3, m1_re, m1_im, m1, r;
  double k44, k11, k14, s0, dop, val1, val2, res_pow;

/* Matrix arrays */
  float ***M_in;
  float **M_avg;
  float **M_odd;
  float **M_dbl;
  float **M_vol;
  float **M_hlx;
  float **M_theta;
  float **M_tau;
  float *TT;

/********************************************************************
********************************************************************/
/* USAGE */

strcpy(UsageHelp,"\ndey_bhattacharya_frery_lopez_rao_4components_decomposition_FP.exe\n");
strcat(UsageHelp,"\nParameters:\n");
strcat(UsageHelp," (string)	-id  	input directory\n");
strcat(UsageHelp," (string)	-od  	output directory\n");
strcat(UsageHelp," (string)	-iodf	input-output data format\n");
strcat(UsageHelp," (int)   	-nwr 	Nwin Row\n");
strcat(UsageHelp," (int)   	-nwc 	Nwin Col\n");
strcat(UsageHelp," (int)   	-ofr 	Offset Row\n");
strcat(UsageHelp," (int)   	-ofc 	Offset Col\n");
strcat(UsageHelp," (int)   	-fnr 	Final Number of Row\n");
strcat(UsageHelp," (int)   	-fnc 	Final Number of Col\n");
strcat(UsageHelp,"\nOptional Parameters:\n");
strcat(UsageHelp," (string)	-mask	mask file (valid pixels)\n");
strcat(UsageHelp," (string)	-errf	memory error file\n");
strcat(UsageHelp," (noarg) 	-help	displays this message\n");
strcat(UsageHelp," (noarg) 	-data	displays the help concerning Data Format parameter\n");

/********************************************************************
********************************************************************/

strcpy(UsageHelpDataFormat,"\nPolarimetric Input-Output Data Format\n\n");
for (ii=0; ii<NPolType; ii++) CreateUsageHelpDataFormatInput(PolTypeConf[ii]); 
strcat(UsageHelpDataFormat,"\n");

/********************************************************************
********************************************************************/
/* PROGRAM START */

if(get_commandline_prm(argc,argv,"-help",no_cmd_prm,NULL,0,UsageHelp)) {
  printf("\n Usage:\n%s\n",UsageHelp); exit(1);
}
if(get_commandline_prm(argc,argv,"-data",no_cmd_prm,NULL,0,UsageHelpDataFormat)) {
  printf("\n Usage:\n%s\n",UsageHelpDataFormat); exit(1);
}

if(argc < 19) {
    edit_error("Not enough input arguments\n Usage:\n",UsageHelp);
  } else {
    get_commandline_prm(argc, argv, "-id",  str_cmd_prm, in_dir, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-od",  str_cmd_prm, out_dir, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-iodf",str_cmd_prm, PolType, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-nwr", int_cmd_prm, &NwinL, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-nwc", int_cmd_prm, &NwinC, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-ofr", int_cmd_prm, &Off_lig, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-ofc", int_cmd_prm, &Off_col, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-fnr", int_cmd_prm, &Sub_Nlig, 1, UsageHelp);
    get_commandline_prm(argc, argv, "-fnc", int_cmd_prm, &Sub_Ncol, 1, UsageHelp);
 
    get_commandline_prm(argc,argv,"-errf",str_cmd_prm,file_memerr,0,UsageHelp);

    MemoryAlloc = -1; MemoryAlloc = CheckFreeMemory();
    MemoryAlloc = my_max(MemoryAlloc,1000);

    PSP_Threads = omp_get_max_threads();
    if (PSP_Threads <= 2) {
      PSP_Threads = 1;
    } else{
      PSP_Threads = PSP_Threads - 1;
    }
    omp_set_num_threads(PSP_Threads);

    FlagValid = 0;strcpy(file_valid,"");
    get_commandline_prm(argc,argv,"-mask",str_cmd_prm,file_valid,0,UsageHelp);
    if (strcmp(file_valid,"") != 0) FlagValid = 1;

    Config = 0;
    for (ii=0; ii<NPolType; ii++) if (strcmp(PolTypeConf[ii],PolType) == 0) Config = 1;
    if (Config == 0) edit_error("\nWrong argument in the Polarimetric Data Format\n",UsageHelpDataFormat);
  }

  if (strcmp(PolType,"S2")==0) strcpy(PolType,"S2T3");

/********************************************************************
********************************************************************/

  check_dir(in_dir);
  check_dir(out_dir);
  if (FlagValid == 1) check_file(file_valid);

  NwinLM1S2 = (NwinL - 1) / 2;
  NwinCM1S2 = (NwinC - 1) / 2;

/* INPUT/OUPUT CONFIGURATIONS */
  read_config(in_dir, &Nlig, &Ncol, PolarCase, PolarType);
  
/* POLAR TYPE CONFIGURATION */
  PolTypeConfig(PolType, &NpolarIn, PolTypeIn, &NpolarOut, PolTypeOut, PolarType);
  
  file_name_in = matrix_char(NpolarIn,1024); 

/* INPUT/OUTPUT FILE CONFIGURATION */
  init_file_name(PolTypeIn, in_dir, file_name_in);

/* INPUT FILE OPENING*/
  for (Np = 0; Np < NpolarIn; Np++)
  if ((in_datafile[Np] = fopen(file_name_in[Np], "rb")) == NULL)
    edit_error("Could not open input file : ", file_name_in[Np]);

  if (FlagValid == 1) 
    if ((in_valid = fopen(file_valid, "rb")) == NULL)
      edit_error("Could not open input file : ", file_valid);

/* OUTPUT FILE OPENING*/
  sprintf(file_name, "%sDey_FP_Odd.bin", out_dir);
  if ((out_odd = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);

  sprintf(file_name, "%sDey_FP_Dbl.bin", out_dir);
  if ((out_dbl = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);

  sprintf(file_name, "%sDey_FP_Vol.bin", out_dir);
  if ((out_vol = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);
  
  sprintf(file_name, "%sDey_FP_Hlx.bin", out_dir);
  if ((out_hlx = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);
  
  sprintf(file_name, "%sDey_FP_Theta.bin", out_dir);
  if ((out_theta = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);
  
  sprintf(file_name, "%sDey_FP_Tau.bin", out_dir);
  if ((out_tau = fopen(file_name, "wb")) == NULL)
    edit_error("Could not open input file : ", file_name);
  
/********************************************************************
********************************************************************/
/* MEMORY ALLOCATION */
/*
MemAlloc = NBlockA*Nlig + NBlockB
*/ 

/* Local Variables */
  NBlockA = 0; NBlockB = 0;
  /* Mask */ 
  NBlockA += Sub_Ncol + NwinC;
  NBlockB += NwinL * (Sub_Ncol + NwinC);

  /* Modd = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB += 0;
  /* Mdbl = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB += 0;
  /* Mvol = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB += 0;
  /* Mhlx = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB = 0;
  /* Mtheta = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB = 0;
  /* Mtau = Nlig*Sub_Ncol */
  NBlockA += Sub_Ncol; NBlockB = 0;
  /* Min = NpolarOut*Nlig*Sub_Ncol */
  NBlockA += NpolarOut * (Ncol + NwinC);
  NBlockB += NpolarOut * NwinL * (Ncol + NwinC);
  /* Mavg = NpolarOut */
  NBlockA += 0; NBlockB += NpolarOut * Sub_Ncol;
  
/* Reading Data */
  NBlockB += Ncol + 2 * Ncol + NpolarIn * 2 * Ncol + NpolarOut * NwinL * (Ncol + NwinC);
  memory_alloc(file_memerr, Sub_Nlig, NwinL, &NbBlock, NligBlock, NBlockA, NBlockB, MemoryAlloc);

/********************************************************************
********************************************************************/
/* MATRIX ALLOCATION */

  _VC_in = vector_float(2 * Ncol);
  _VF_in = vector_float(Ncol);
  _MC_in = matrix_float(4, 2 * Ncol);
  _MF_in = matrix3d_float(NpolarOut, NwinL, Ncol + NwinC);

/*-----------------------------------------------------------------*/   

  Valid = matrix_float(NligBlock[0] + NwinL, Sub_Ncol + NwinC);

  M_in = matrix3d_float(NpolarOut, NligBlock[0] + NwinL, Ncol + NwinC);
  //M_avg = matrix_float(NpolarOut, Sub_Ncol);
  M_odd = matrix_float(NligBlock[0], Sub_Ncol);
  M_dbl = matrix_float(NligBlock[0], Sub_Ncol);
  M_vol = matrix_float(NligBlock[0], Sub_Ncol);
  M_hlx = matrix_float(NligBlock[0], Sub_Ncol);
  M_theta = matrix_float(NligBlock[0], Sub_Ncol);
  M_tau = matrix_float(NligBlock[0], Sub_Ncol);

/********************************************************************
********************************************************************/
/* MASK VALID PIXELS (if there is no MaskFile */
  if (FlagValid == 0) 
#pragma omp parallel for private(col)
    for (lig = 0; lig < NligBlock[0] + NwinL; lig++) 
      for (col = 0; col < Sub_Ncol + NwinC; col++) 
        Valid[lig][col] = 1.;
 
/********************************************************************
********************************************************************/
/* SPANMIN / SPANMAX DETERMINATION */
for (Np = 0; Np < NpolarIn; Np++) rewind(in_datafile[Np]);
if (FlagValid == 1) rewind(in_valid);

Span = 0.;
SpanMin = INIT_MINMAX;
SpanMax = -INIT_MINMAX;
  
for (Nb = 0; Nb < NbBlock; Nb++) {
  ligDone = 0;
  if (NbBlock > 2) { printf("%f\r", 100. * Nb / (NbBlock - 1)); fflush(stdout); }

  if (FlagValid == 1) read_block_matrix_float(in_valid, Valid, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);

  if (strcmp(PolType,"S2")==0) {
    read_block_S2_noavg(in_datafile, M_in, PolTypeOut, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
  } else {
  /* Case of C,T or I */
    read_block_TCI_noavg(in_datafile, M_in, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
  }
  if (strcmp(PolTypeOut,"C3")==0) C3_to_T3(M_in, NligBlock[Nb], Sub_Ncol + NwinC, 0, 0);

#pragma omp parallel for private(col, M_avg) firstprivate(Span) shared(ligDone, SpanMin, SpanMax)
  for (lig = 0; lig < NligBlock[Nb]; lig++) {
    ligDone++;
    if (omp_get_thread_num() == 0) PrintfLine(ligDone,NligBlock[Nb]);
    M_avg = matrix_float(NpolarOut,Sub_Ncol);
    average_TCI(M_in, Valid, NpolarOut, M_avg, lig, Sub_Ncol, NwinL, NwinC, NwinLM1S2, NwinCM1S2);
    for (col = 0; col < Sub_Ncol; col++) {
      if (Valid[NwinLM1S2 + lig][NwinCM1S2 + col] == 1.) {
        Span = M_avg[T311][col] + M_avg[T322][col] + M_avg[T333][col];
        if (Span >= SpanMax) SpanMax = Span;
        if (Span <= SpanMin) SpanMin = Span;
      }       
    }
    free_matrix_float(M_avg,NpolarOut);
  }
} // NbBlock

  if (SpanMin < eps) SpanMin = eps;

/********************************************************************
********************************************************************/
/* DATA PROCESSING */
for (Np = 0; Np < NpolarIn; Np++) rewind(in_datafile[Np]);
if (FlagValid == 1) rewind(in_valid);

for (Nb = 0; Nb < NbBlock; Nb++) {
  ligDone = 0;
  if (NbBlock > 2) {printf("%f\r", 100. * Nb / (NbBlock - 1));fflush(stdout);}
 
  if (FlagValid == 1) read_block_matrix_float(in_valid, Valid, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);

  if (strcmp(PolType,"S2")==0) {
    read_block_S2_noavg(in_datafile, M_in, PolTypeOut, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
    } else {
  /* Case of C,T or I */
    read_block_TCI_noavg(in_datafile, M_in, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
    }
  if (strcmp(PolTypeOut,"C3")==0) C3_to_T3(M_in, NligBlock[Nb], Sub_Ncol + NwinC, 0, 0);

tau = theta = 0.;
Ps = Pd = Pv = Pc = 0.;
det_re = det_im = trace = trace3 = m1_re = m1_im = m1 = r = 0.;
k44 = k11 = k14 = s0 = dop = val1 = val2 = res_pow = 0.;
#pragma omp parallel for private(col, Np, M_avg) firstprivate(theta, tau, Ps, Pd, Pv, Pc, det_re, det_im, trace, trace3, m1_re, m1_im, m1, r, k44, k11, k14, s0, dop, val1, val2, res_pow) shared(ligDone)
  for (lig = 0; lig < NligBlock[Nb]; lig++) {
    ligDone++;
    if (omp_get_thread_num() == 0) PrintfLine(ligDone,NligBlock[Nb]);
    TT = vector_float(NpolarOut);
    M_avg = matrix_float(NpolarOut,Sub_Ncol);
    average_TCI(M_in, Valid, NpolarOut, M_avg, lig, Sub_Ncol, NwinL, NwinC, NwinLM1S2, NwinCM1S2);
    for (col = 0; col < Sub_Ncol; col++) {
      if (Valid[NwinLM1S2+lig][NwinCM1S2+col] == 1.) {
        for (Np = 0; Np < NpolarOut; Np++) TT[Np] = M_avg[Np][col];
        det_re = - TT[T333] * TT[T312_im] *TT[T312_im]  - 2. * TT[T312_im] * TT[T313_re] * TT[T323_im] + TT[T322] * TT[T313_im] *TT[T313_im]
                 - TT[T333] * TT[T312_re] * TT[T312_re] + 2. * TT[T312_re] * TT[T313_re] * TT[T323_re] - TT[T322] * TT[T313_re] * TT[T313_re]
                 - TT[T311] * TT[T323_im] * TT[T323_im] - TT[T311] * TT[T323_re] * TT[T323_re] + TT[T311] * TT[T322] * TT[T333];
        det_im =  - 2. * (TT[T312_im] * TT[T313_im] * TT[T323_im] - TT[T313_im] * TT[T312_re] * TT[T323_re]  + TT[T322] * TT[T313_im] * TT[T313_re]);
        trace = TT[T311] + TT[T322] + TT[T333];
        trace3 = trace * trace * trace;
        m1_re = 1. - 27. * det_re / trace3;
        m1_im = 0. - 27. * det_im / trace3;
        r = sqrt(m1_re * m1_re + m1_im * m1_im);
        theta = atan2(m1_im, m1_re); /* convert to polar */
        m1 = sqrt(r) * cos(theta / 2); /* take square root and real part */
        k44 = (-TT[T311] + TT[T322] + TT[T333]) / 2.;
        k11 = trace / 2.;
        k14 = TT[T323_im];
        s0 = trace;
        dop = m1;
        val1 = (4. * dop * k11 * k44) / (k44 * k44 - (1. + 4. * dop + dop) * k11 * k11);
        val2 = fabs(k14) / k11; /* abs is for ints only! */
        theta = atan(val1); /* separation for surface and dbl */
        tau = atan(val2);   /* separation for helix */

        Pc = dop * s0 * (sin(2. * tau));
        Pv = (1. - dop) * s0;
        res_pow = s0 - (Pc + Pv);
        Ps = (res_pow / 2.) * (1. + sin(2. * theta));
        Pd = (res_pow / 2.) * (1. - sin(2. * theta));

        if (Ps < SpanMin) Ps = SpanMin;
        if (Pd < SpanMin) Pd = SpanMin;
        if (Pv < SpanMin) Pv = SpanMin;
        if (Pc < SpanMin) Pc = SpanMin;

        if (Ps > SpanMax) Ps = SpanMax;
        if (Pd > SpanMax) Pd = SpanMax;
        if (Pv > SpanMax) Pv = SpanMax;
        if (Pc > SpanMax) Pc = SpanMax;

        M_odd[lig][col] = Ps;
        M_dbl[lig][col] = Pd;
        M_vol[lig][col] = Pv;
        M_hlx[lig][col] = Pc;
        M_theta[lig][col] = theta * 180. / pi;
        M_tau[lig][col] = tau * 180. / pi;

        } else {
        M_odd[lig][col] = 0.;
        M_dbl[lig][col] = 0.;
        M_vol[lig][col] = 0.;
        M_hlx[lig][col] = 0.;
        M_theta[lig][col] = 0.;
        M_tau[lig][col] = 0.;
        }
      }
    free_matrix_float(M_avg,NpolarOut);
    free_vector_float(TT);
    }

  write_block_matrix_float(out_odd, M_odd, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  write_block_matrix_float(out_dbl, M_dbl, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  write_block_matrix_float(out_vol, M_vol, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  write_block_matrix_float(out_hlx, M_hlx, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  write_block_matrix_float(out_theta, M_theta, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  write_block_matrix_float(out_tau, M_tau, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);

  } // NbBlock

/********************************************************************
********************************************************************/
/* MATRIX FREE-ALLOCATION */
/*
  free_matrix_float(Valid, NligBlock[0]);

  free_matrix3d_float(M_avg, NpolarOut, NligBlock[0]);
  free_matrix_float(M_odd, NligBlock[0]);
  free_matrix_float(M_dbl, NligBlock[0]);
  free_matrix_float(M_vol, NligBlock[0]);
  free_matrix_float(M_hlx, NligBlock[0]);
  free_matrix_float(M_theta, NligBlock[0]);
  free_matrix_float(M_tau, NligBlock[0]);
*/  
/********************************************************************
********************************************************************/
/* INPUT FILE CLOSING*/
  for (Np = 0; Np < NpolarIn; Np++) fclose(in_datafile[Np]);
  if (FlagValid == 1) fclose(in_valid);

/* OUTPUT FILE CLOSING*/
  fclose(out_odd);
  fclose(out_dbl);
  fclose(out_vol);
  fclose(out_hlx);
  fclose(out_theta);
  fclose(out_tau);
  
/********************************************************************
********************************************************************/

  return 1;
}
