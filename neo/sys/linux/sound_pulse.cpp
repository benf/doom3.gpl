/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2011 Benjamin Franzke <benjaminfranzke@googlemail.com>
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../idlib/precompiled.h"
#include "../../sound/snd_local.h"
#include "../posix/posix_public.h"
#include "sound.h"

#include <pulse/simple.h>
#include <pulse/error.h>

void idAudioHardwarePULSE::Release() {
	if ( m_buffer ) {
		free( m_buffer );
		m_buffer = NULL;
	}

	if ( m_pulse ) {
		pa_simple_free( m_pulse );
	}
}

void idAudioHardwarePULSE::InitFailed() {
	Release();
	cvarSystem->SetCVarBool( "s_noSound", true );
	common->Warning( "sound subsystem disabled\n" );
	common->Printf( "--------------------------------------\n" );
}

bool idAudioHardwarePULSE::Initialize( void ) {
	int err;

	common->Printf( "------ Pulse Sound Initialization -----\n" );
	
	m_spec.format = PA_SAMPLE_S16LE;
	m_spec.rate = 44100;
	m_spec.channels = m_channels = idSoundSystemLocal::s_numberOfSpeakers.GetInteger();

	if (!(m_pulse = pa_simple_new(NULL, "doom3", PA_STREAM_PLAYBACK, NULL,
				      "playback", &m_spec, NULL, NULL, &err))) {
		common->Printf( "pa_simple_new() failed: %s\n", pa_strerror(err));
		InitFailed();
		return false;

	}
	common->Printf( "opened pulseaudio connection for playback\n" );

	// allocate the final mix buffer
	m_buffer_size = MIXBUFFER_SAMPLES * m_spec.channels * 2;
	m_buffer = malloc( m_buffer_size );
	common->Printf( "allocated a mix buffer of %d bytes\n", m_buffer_size );

	return true;
}

idAudioHardwarePULSE::~idAudioHardwarePULSE() {
	common->Printf( "----------- Pulse Shutdown ------------\n" );
	Release();
	common->Printf( "--------------------------------------\n" );
}

int idAudioHardwarePULSE::GetMixBufferSize() {
	return m_buffer_size;
}

short* idAudioHardwarePULSE::GetMixBuffer() {
	return (short *) m_buffer;
}

bool idAudioHardwarePULSE::Flush( void ) {
	return true;
}

void idAudioHardwarePULSE::Write( bool flushing ) {
	int err;

	if ( flushing ) 
		return;

	/* if running after the mix loop, then we have a full buffer to write out */
	if ( pa_simple_write( m_pulse, m_buffer, m_buffer_size, &err ) < 0 )
		Sys_Printf( "pa_simple_write() failed: %s\n", pa_strerror(err) );
}
