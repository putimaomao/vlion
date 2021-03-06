/***************************************************************************
 *
 * Author: "Sjors H.W. Scheres"
 * MRC Laboratory of Molecular Biology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This complete copyright notice must be included in any revised version of the
 * source code. Additional authorship citations may be added, but existing
 * author citations must be preserved.
 ***************************************************************************/

#ifndef PARTICLE_POLISHER_H_
#define PARTICLE_POLISHER_H_
#include "src/image.h"
#include "src/metadata_table.h"
#include "src/exp_model.h"
#include <src/fftw.h>
#include <src/time.h>
#include <src/mask.h>
#include <src/funcs.h>
#include <src/backprojector.h>
#include <src/ctf.h>
#include <src/postprocessing.h>

#define LINEAR_FIT 0
#define LOGARITHMIC_FIT 1
#define SQRT_FIT 2
#define NO_FIT 3

class ParticlePolisher
{
public:

	// I/O Parser
	IOParser parser;

	// Verbosity
	int verb;

	// Input & Output rootname
	FileName fn_in, fn_out, fn_sym, fn_mask;

	// The experimental model
	Experiment exp_model;

	// Standard deviation for a Gaussian-weight on the distance between particles in the micrograph
	DOUBLE sigma_neighbour_distance;

	// Maximum resolution in pre-frame reconstructions
	DOUBLE perframe_highres;

	// Flag to indicate all calculations have to be repeated from scratch
	// if false, then intermediate files are re-read from disc and earlier calculations are skipped
	bool do_start_all_over;

	// First and last frame numbers to include in the average, Also step if one had used --avg_movie_frames in the extraction
	int first_frame, last_frame, step_frame;

	// Which fitting mode (lienar/logarithmic/nofit)
	int fitting_mode;

	// Running average window with used for the determination of the frame movements
	// This will be used to exclude the first and last few frames from the fit (but not from the polsihing!)
	int running_average_width;

	// CTF stuff for the reconstructions
	bool do_ctf, ctf_phase_flipped, only_flip_phases, intact_ctf_first_peak;

	// Pixel size (for B-factors)
	DOUBLE angpix;

	// Original image size
	int ori_size;

	// Skip B-factor weighting
	bool do_weighting;

	// Minimum resolution (in Angstrom) for fitting of B-factor in Guinier plot
	DOUBLE fit_minres;

	// Width of a running average window for the single-frame reconstructions
	int frame_running_average;

	// Vector with the B-factors for all individual frames
	MultidimArray<DOUBLE> perframe_bfactors;

	// Fitted movement coordinates for all input images
	MultidimArray<DOUBLE> fitted_movements;

	// Image with the mask (used for relative weighting of each frame)
	Image<DOUBLE> Imask;

	// FSC curve of the masked, averages of all single-frame reconstructions
	MultidimArray<DOUBLE> fsc_average;

	// Metadatatable with the information from the polished particles
	MetaDataTable MDshiny;

	// Tabulated sin and cosine functions for shifts in Fourier space
	TabSine tab_sin;
	TabCosine tab_cos;

	// Reference volume reconstructed from the initially-polished particles to be used for per-particle CTF-refinement and beamtilt-refinement
	Projector PPrefvol_half1, PPrefvol_half2;

	// Normalise the polished particles?
	bool do_normalise;

	// Subtract a ramping background in the normalisation?
	bool do_ramp;

	// Radius of the background-circle for noise normalisation (in pixels)
	int bg_radius;

	// Sigma-levels for dust removal
	DOUBLE white_dust_stddev, black_dust_stddev;

	// Maximum useful resolution in the reconstruction
	DOUBLE maxres_model;

	// Maximum beam tilt to analyse, and step-size to sample in X and Y
	DOUBLE beamtilt_max, beamtilt_step;
	// Number of sampled beamtilts
	int nr_sampled_beam_tilts;

	// Names of the data sets to be separated in the beamtilt refinement
	std::vector<FileName> fn_beamtilt_groups;

	// Minimum resolution to take beamtilt into account
	DOUBLE minres_beamtilt;

	// Weighted squared-differences for all beamtilts
	MultidimArray<DOUBLE> diff2_beamtilt;

	// Weighted squared-differences for all defocus values
	MultidimArray<DOUBLE> defocus_shift_allmics;

	// Optimal beamtilts for each data set
	std::vector<Matrix1D<DOUBLE> > best_beamtilts;

	// Per-particle CTF optimisation
	DOUBLE defocus_shift_max, defocus_shift_step;


public:
	// Read command line arguments
	void read(int argc, char **argv);

	// Print usage instructions
	void usage();

	// Initialise some stuff after reading
	void initialise();

	// General Running
	void run();

	// Fit the beam-induced translations for all average micrographs
	void fitMovementsAllMicrographs();

	// Fit a function through th observed movements
	void fitMovementsOneMicrograph(long int imic);

	// Get the B-factor for all single-frame reconstruction
	void calculateAllSingleFrameReconstructionsAndBfactors();

	// Read/write of STAR file with all per-frame B-factors
	bool readStarFileBfactors(FileName fn_star);
	void writeStarFileBfactors(FileName fn_star);

	// Write out an additional STAR file with the resolution-dependent, relative weights per frame
	void writeStarFileRelativeWeights(FileName fn_star);

	// Get the B-factor for a single-frame reconstruction
	void calculateSingleFrameReconstruction(int iframe, int ihalf);

	// Run standard post-processing (only unmasked FSC  on the single-frame reconstruction.
	void postProcessSingleFrameReconstruction(int this_frame);

	// Calculate the B-factors for a single-frame reconstruction
	void calculateBfactorSingleFrameReconstruction(int this_frame, DOUBLE &bfactor, DOUBLE &offset, DOUBLE &corr_coeff);

	// Calculate the average of all single-frame rconstructions (for a given half)
	void calculateAverageAllSingleFrameReconstructions(int ihalf);

	// Movie frame re-alignment for a single micrograph
	void polishParticlesOneMicrograph(long int imic);

	// Movie frame re-alignment for all micrographs
	void polishParticlesAllMicrographs();

	// Write out the resulting STAR files
	void writeStarFilePolishedParticles();

	// Reconstruct the two independent halves of the shiny particles
	void reconstructShinyParticlesAndFscWeight(int ipass);

	// Reconstruct one half of the shiny particles
	void reconstructShinyParticlesOneHalf(int ihalf);

	// Optimize the beam tilt and defocus for all beamtilt groups and/or micrographs
	void optimiseBeamTiltAndDefocus();

	// Optimisation for each micrograph (may be run in parallel)
	void optimiseBeamTiltAndDefocusOneMicrograph(int imic);

	// After optimising, one general function to set results in the MetaDataTable (because optimisation may have been done in parallel)
	void applyOptimisedBeamTiltsAndDefocus();

	// Optimise beamtilt separately for datasets in different directories
	void getBeamTiltGroups();

	// Initialise some arrays for parallelisation purposes
	void initialiseSquaredDifferenceVectors();
};



#endif /* PARTICLE_POLISHER_H_ */
