%/******************************************************************************\
% * Copyright (c) 2004-2006
% *
% * Author(s):
% *	Volker Fischer
% *
%\******************************************************************************/

% Filter for ratios close to 1 -------------------------------------------------
% Fixed for sample-rate conversiones of R ~ 1
I = 10; % D = I
% Number of taps per poly-phase
NoTapsP = 12;
% Cut-off frequency
fc = 0.97 / I;
% MMSE filter-design and windowing
h = I * firls(I * NoTapsP - 1, [0 fc fc 1], [1 1 0 0]) .* kaiser(I * NoTapsP, 5)';


% Export coefficiants to file ****************************************
fid = fopen('resamplefilter.h', 'w');

fprintf(fid, '/* Automatically generated file with MATLAB */\n');
fprintf(fid, '/* File name: "ResampleFilter.m" */\n');
fprintf(fid, '/* Filter taps in time-domain */\n\n');

fprintf(fid, '#ifndef _RESAMPLEFILTER_H_\n');
fprintf(fid, '#define _RESAMPLEFILTER_H_\n\n');

fprintf(fid, '#define NUM_TAPS_PER_PHASE           ');
fprintf(fid, int2str(NoTapsP));
fprintf(fid, '\n');
fprintf(fid, '#define INTERP_DECIM_I_D             ');
fprintf(fid, int2str(I));
fprintf(fid, '\n\n\n');

% Write filter taps
fprintf(fid, '/* Filter for ratios close to 1 */\n');
fprintf(fid, 'static float fResTaps1To1[INTERP_DECIM_I_D][NUM_TAPS_PER_PHASE] = {\n');
for i = 1:I
	hTemp = h(i:I:end) ;
	fprintf(fid, '{\n');
	fprintf(fid, '	%.20ff,\n', hTemp(1:end - 1));
	fprintf(fid, '	%.20ff\n', hTemp(end));
	fprintf(fid, '},\n');
end
fprintf(fid, '};\n\n\n');

fprintf(fid, '#endif	/* _RESAMPLEFILTER_H_ */\n');
fclose(fid);
