/*
*	Copyright (C) 2010 Thorsten Liebig (Thorsten.Liebig@gmx.de)
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "engine_sse.h"

//! \brief construct an Engine_sse instance
//! it's the responsibility of the caller to free the returned pointer
Engine_sse* Engine_sse::New(const Operator_sse* op)
{
	Engine_sse* e = new Engine_sse(op);
	e->Init();
	return e;
}

Engine_sse::Engine_sse(const Operator_sse* op) : Engine(op)
{
	Op = op;
	for (int n=0;n<3;++n)
	{
		numLines[n] = Op->GetNumberOfLines(n);
	}
}

Engine_sse::~Engine_sse()
{
	this->Reset();
}

void Engine_sse::Init()
{
	numTS = 0;
	f4_volt = Create_N_3DArray_v4sf(numLines);
	f4_curr = Create_N_3DArray_v4sf(numLines);
	volt = 0; // not used
	curr = 0; // not used
}

void Engine_sse::Reset()
{
	Delete_N_3DArray_v4sf(f4_volt,numLines);
	f4_volt = 0;
	Delete_N_3DArray_v4sf(f4_curr,numLines);
	f4_curr = 0;
}

void Engine_sse::UpdateVoltages()
{
	unsigned int pos[3];
	bool shift[2];
	f4vector temp;

	for (pos[0]=0;pos[0]<numLines[0];++pos[0])
	{
		shift[0]=pos[0];
		for (pos[1]=0;pos[1]<numLines[1];++pos[1])
		{
			shift[1]=pos[1];
			for (pos[2]=0;pos[2]<ceil(numLines[2]/4);++pos[2])
			{
				// x-polarization
				temp.f[0] = f4_curr[1][pos[0]][pos[1]][pos[2]-(bool)pos[2]].f[3];
				temp.f[1] = f4_curr[1][pos[0]][pos[1]][pos[2]].f[0];
				temp.f[2] = f4_curr[1][pos[0]][pos[1]][pos[2]].f[1];
				temp.f[3] = f4_curr[1][pos[0]][pos[1]][pos[2]].f[2];
				f4_volt[0][pos[0]][pos[1]][pos[2]].v *= Op->f4_vv[0][pos[0]][pos[1]][pos[2]].v;
				f4_volt[0][pos[0]][pos[1]][pos[2]].v += Op->f4_vi[0][pos[0]][pos[1]][pos[2]].v * ( f4_curr[2][pos[0]][pos[1]][pos[2]].v - f4_curr[2][pos[0]][pos[1]-shift[1]][pos[2]].v - f4_curr[1][pos[0]][pos[1]][pos[2]].v + temp.v );

				// y-polarization
				temp.f[0] = f4_curr[0][pos[0]][pos[1]][pos[2]-(bool)pos[2]].f[3];
				temp.f[1] = f4_curr[0][pos[0]][pos[1]][pos[2]].f[0];
				temp.f[2] = f4_curr[0][pos[0]][pos[1]][pos[2]].f[1];
				temp.f[3] = f4_curr[0][pos[0]][pos[1]][pos[2]].f[2];
				f4_volt[1][pos[0]][pos[1]][pos[2]].v *= Op->f4_vv[1][pos[0]][pos[1]][pos[2]].v;
				f4_volt[1][pos[0]][pos[1]][pos[2]].v += Op->f4_vi[1][pos[0]][pos[1]][pos[2]].v * ( f4_curr[0][pos[0]][pos[1]][pos[2]].v - temp.v - f4_curr[2][pos[0]][pos[1]][pos[2]].v + f4_curr[2][pos[0]-shift[0]][pos[1]][pos[2]].v);

				// z-polarization
				f4_volt[2][pos[0]][pos[1]][pos[2]].v *= Op->f4_vv[2][pos[0]][pos[1]][pos[2]].v;
				f4_volt[2][pos[0]][pos[1]][pos[2]].v += Op->f4_vi[2][pos[0]][pos[1]][pos[2]].v * ( f4_curr[1][pos[0]][pos[1]][pos[2]].v - f4_curr[1][pos[0]-shift[0]][pos[1]][pos[2]].v - f4_curr[0][pos[0]][pos[1]][pos[2]].v + f4_curr[0][pos[0]][pos[1]-shift[1]][pos[2]].v);
			}
		}
	}
}

void Engine_sse::UpdateCurrents()
{
	unsigned int pos[5];
	f4vector temp;

	for (pos[0]=0;pos[0]<numLines[0]-1;++pos[0])
	{
		for (pos[1]=0;pos[1]<numLines[1]-1;++pos[1])
		{
			for (pos[2]=0;pos[2]<ceil(numLines[2]/4);++pos[2]) // FIXME is this correct?
			{
				// x-pol
				temp.f[0] = f4_volt[1][pos[0]][pos[1]][pos[2]].f[1];
				temp.f[1] = f4_volt[1][pos[0]][pos[1]][pos[2]].f[2];
				temp.f[2] = f4_volt[1][pos[0]][pos[1]][pos[2]].f[3];
				temp.f[3] = f4_volt[1][pos[0]][pos[1]][pos[2]+1].f[0]; // FIXME outside sim area
				f4_curr[0][pos[0]][pos[1]][pos[2]].v *= Op->f4_ii[0][pos[0]][pos[1]][pos[2]].v;
				f4_curr[0][pos[0]][pos[1]][pos[2]].v += Op->f4_iv[0][pos[0]][pos[1]][pos[2]].v * ( f4_volt[2][pos[0]][pos[1]][pos[2]].v - f4_volt[2][pos[0]][pos[1]+1][pos[2]].v - f4_volt[1][pos[0]][pos[1]][pos[2]].v + temp.v);

				// y-pol
				temp.f[0] = f4_volt[0][pos[0]][pos[1]][pos[2]].f[1];
				temp.f[1] = f4_volt[0][pos[0]][pos[1]][pos[2]].f[2];
				temp.f[2] = f4_volt[0][pos[0]][pos[1]][pos[2]].f[3];
				temp.f[3] = f4_volt[0][pos[0]][pos[1]][pos[2]+1].f[0]; // FIXME outside sim area
				f4_curr[1][pos[0]][pos[1]][pos[2]].v *= Op->f4_ii[1][pos[0]][pos[1]][pos[2]].v;
				f4_curr[1][pos[0]][pos[1]][pos[2]].v += Op->f4_iv[1][pos[0]][pos[1]][pos[2]].v * ( f4_volt[0][pos[0]][pos[1]][pos[2]].v - temp.v - f4_volt[2][pos[0]][pos[1]][pos[2]].v + f4_volt[2][pos[0]+1][pos[1]][pos[2]].v);

				// z-pol
				f4_curr[2][pos[0]][pos[1]][pos[2]].v *= Op->f4_ii[2][pos[0]][pos[1]][pos[2]].v;
				f4_curr[2][pos[0]][pos[1]][pos[2]].v += Op->f4_iv[2][pos[0]][pos[1]][pos[2]].v * ( f4_volt[1][pos[0]][pos[1]][pos[2]].v - f4_volt[1][pos[0]+1][pos[1]][pos[2]].v - f4_volt[0][pos[0]][pos[1]][pos[2]].v + f4_volt[0][pos[0]][pos[1]+1][pos[2]].v);
			}
		}
	}
}
