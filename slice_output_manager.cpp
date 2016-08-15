/*
 * SpectRE - A Spectral Code for Reheating
 * Copyright (C) 2009-2010 Hal Finkel, Nathaniel Roth and Richard Easther
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "slice_output_manager.hpp"
#include "host_field.hpp"

using namespace std;

template <typename R>
void slice_output_manager<R>::add_outputter(std::string varname, var_func vf)
{
	outputters.push_back(
		new slice_outputter<R>(fs, mp, ts, slicelength, varname, vf, sliceflt)
	);
}

/**
 * @page slices Binary Slices
 * Binary slices are optionally generated for many different variables.
 * Single-precision floating point format is used regardless of the precision used
 * for computation. The "length" parameter indicates the length of the side of the
 * grid from which the slice is taken, @b not the size of the output slice if
 * "skip" is \> 0. "skip" is the number of grid points inbetween output points.
 * If averaging is active, the skipped points are averaged over instead of actually
 * being skipped.
 */

template <typename R>
void slice_output_manager<R>::output()
{
	if (!outputters.size()) {
		return;
	}
	
	gc.compute();
	
	for (typename vector< slice_outputter<R> * >::iterator it = outputters.begin();
		it != outputters.end(); ++it) {
		(*it)->begin(bin_idx);
	}
	
	phi.switch_state(position);
	chi.switch_state(position);
	
	phidot.switch_state(position);
	chidot.switch_state(position);

	auto phi_host = host_field(fs);
	auto chi_host = host_field(fs);
	auto phidot_host = host_field(fs);
	auto chidot_host = host_field(fs);
	auto phigradx_host = host_field(fs);
	auto phigrady_host = host_field(fs);
	auto phigradz_host = host_field(fs);
	auto chigradx_host = host_field(fs);
	auto chigrady_host = host_field(fs);
	auto chigradz_host = host_field(fs);

	phi.download(phi_host);
	chi.download(chi_host);
	phidot.download(phidot_host);
	chidot.download(chidot_host);
	gc.phigradx.download(phigradx_host);
	gc.phigrady.download(phigrady_host);
	gc.phigradz.download(phigradz_host);
	gc.chigradx.download(chigradx_host);
	gc.chigrady.download(chigrady_host);
	gc.chigradz.download(chigradz_host);

	for (int i = 0; i < (slicedim > 2 ? slicelength : 1); i += sliceskip) {
		for (int j = 0; j < (slicedim > 1 ? slicelength : 1); j += sliceskip) {
			for (int k = 0; k < slicelength; k += sliceskip) {
				if (sliceaverage) {
					// Average over all "skipped" points
					for (int x = i; x < i + sliceskip && x < fs.n; ++x) {
						for (int y = j; y < j + sliceskip && y < fs.n; ++y) {
							for (int z = k; z < k + sliceskip && z < fs.n; ++z) {
								int fdx = z + phi.ldl*(y + fs.n*x);
								int idx = z + gc.phigradx.ldl*(y + fs.n*x);
								for (auto it = outputters.begin();
								     it != outputters.end(); ++it) {
									(*it)->accumulate(phi_host.data()[fdx],
											  chi_host.data()[fdx],
											  phidot_host.data()[fdx],
											  chidot_host.data()[fdx],
											  phigradx_host.data()[idx],
											  chigradx_host.data()[idx],
											  phigrady_host.data()[idx],
											  chigrady_host.data()[idx],
											  phigradz_host.data()[idx],
											  chigradz_host.data()[idx]);
								}
							}
						}
					}

					for (auto it = outputters.begin();
					     it != outputters.end(); ++it) {
						(*it)->advance();
					}	
				}
				else {
					int fdx = k + phi.ldl*(j + fs.n*i);
					int idx = k + gc.phigradx.ldl*(j + fs.n*i);
					for (auto it = outputters.begin(); it != outputters.end(); ++it) {
						(*it)->accumulate(phi_host.data()[fdx], chi_host.data()[fdx],
								  phidot_host.data()[fdx], chidot_host.data()[fdx],
								  phigradx_host.data()[idx], chigradx_host.data()[idx],
								  phigrady_host.data()[idx], chigrady_host.data()[idx],
								  phigradz_host.data()[idx], chigradz_host.data()[idx]);
						(*it)->advance();
					}
				}
				
				for (auto it = outputters.begin(); it != outputters.end(); ++it) {
					(*it)->flush();
				}
			}
		}
	}
	
	++bin_idx;
}

// Explicit instantiations
template class slice_output_manager<double>;
