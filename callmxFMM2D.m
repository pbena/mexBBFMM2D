% Kernel 
syms r;
kernel = exp(-sqrt(r^0.5)/30); % user input here
make(r,kernel);
%%
clear all  % QH and QHexact must be cleaned before start
% Info on matrices
Nx =80; Ny = 80; N = Nx*Ny;
x = linspace(0,1,Nx);
y = linspace(0,1,Ny);
[xloc,yloc] = meshgrid(x,y);
% Store location in column-wise fashion
xloc = xloc(:);  yloc = yloc(:);
% The right muliplier H, each column is a Nx1 vector 
H = ones(N,100); nCheb = 6;
[QHexact,QH] = mexFMM2D(xloc, yloc,H,nCheb);