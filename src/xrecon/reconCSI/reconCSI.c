/* reconCSI.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* reconCSI.c: CSI recon                                                     */
/*                                                                           */
/* Copyright (C) 2012 Margaret Kritzer                                       */
/*                                                                           */
/* This file is part of Xrecon.                                              */
/*                                                                           */
/* Xrecon is free software: you can redistribute it and/or modify            */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* Xrecon is distributed in the hope that it will be useful,                 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with Xrecon. If not, see <http://www.gnu.org/licenses/>.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/

#include "../Xrecon.h"

void reconCSI(struct data *d)
{

  if(spar(d,"apptype","im2Dcsi"))
    reconCSI2D(d);


  /* Otherwise perform standard 3D recon */
  else if(spar(d,"apptype","im3Dcsi"))
	  reconCSI3D(d);



}
