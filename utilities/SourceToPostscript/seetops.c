/* seetops - make a postscript file that prints Interlisp source files
   using the ^F font tags to render keywords and comments appropriately,
   also renders _ as left arrow and ^ as up arrow

   Written by Darrel J. Van Buer, PhD 2022, released to public domain

   seetops [infile] [outfile]
Arguments omitted or given as - uses stdin/stdout
*/
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* output: header to set up array of fonts and heights.
   Line of input file split into different fonts and output something like
   x y moveto Fx setfont (some text)show Fy setfont (more text)show ...
   I think x position will be fixed at left margin, y decremented from previous line
   by tallest font in this line.  When x gets to bottom margin, output showpage
*/

int main(int argc, char **argv) {
  int heights[7] = {8, 8, 6, 10, 8, 8, 8}, maxHt, chUsed, page = 1,
    curfont = 1, curY, inString, i, nexF;
  char inputline[500], *result, *ctlF, *outPtr, outline[1024];
  FILE *in, *out;
  if(argc > 1) {
    if(!strcmp(argv[1], "-")) in = stdin;
    else {
      in = fopen(argv[1], "r");
      if(in == NULL) {
	fprintf(stderr, "Unable to open '%s': %s\n",
		argv[1], strerror(errno));
	exit(EXIT_FAILURE);
      }
    }
    if(argc > 2) {
      if(!strcmp(argv[2], "-")) out = stdout;
      else {
	out = fopen(argv[2], "w");
	if(out == NULL) {
	  fprintf(stderr, "Unable to open '%s': %s\n",
		  argv[2], strerror(errno));
	  exit(EXIT_FAILURE);
	}
      }
    }
    else out = stdout;
  }
  else {
    in = stdin;
    out = stdout;
  }
       
  fprintf(out, "%%!PS-Adobe 1.0\n\
  /fS /Symbol findfont 8 scalefont def /FS {fS setfont} def\
  /f1 /Courier findfont 8 scalefont def /F1 {f1 setfont} def\
  /f2 /Helvetica-Bold findfont 8 scalefont def /F2 {f2 setfont} def\
  /f3 /Courier findfont 6 scalefont def /F3 {f3 setfont} def\
  /f4 /Helvetica-Bold findfont 10 scalefont def /F4 {f4 setfont} def\
  /f5 /Times-Roman findfont 8 scalefont def /F5 {f5 setfont} def\
  /f6 /Helvetica-Oblique findfont 8 scalefont def /F6 {f6 setfont} def\
  /f7 /Times-Bold findfont 8 scalefont def /F7 {f7 setfont} def\n");
  fprintf(out, "F1 \n");
  curfont = 1;
 newpage:
  fprintf(out, "F1 550 752 moveto (%d) show\n", page++);
  curY = 746;
 newline:
  result = fgets(inputline, sizeof(inputline), in);
  if(feof(in)) {
    fprintf(out, "\nshowpage\n");
    return 0;
  }
  if(inputline[strlen(inputline)-1] == '\n' || (inputline[strlen(inputline)-1] == '\r'))
    inputline[strlen(inputline)-1] = '\0';
  ctlF = result;
  maxHt = heights[curfont - 1];
  while((ctlF = index(ctlF, 6))) {
    nexF = ctlF[1];
    if(nexF && heights[nexF] > maxHt) maxHt = heights[nexF];
    if(!nexF) break;
    ctlF += 2;
  }
  curY -= maxHt;
  chUsed = snprintf(outline, sizeof(outline), "40 %d moveto ", curY);
  outPtr = outline + chUsed;
  inString = 0; // flag for accumulating string to output
  for(i = 0; result[i]; i++) {
    switch(result[i]) {
    case 6: /* font tag */
      nexF = result[++i];
      if(inString) {
	chUsed = snprintf(outPtr, sizeof(outline) - (size_t)(outPtr - outline), ")show ");
	outPtr += chUsed;
	inString = 0;
      }
      chUsed = snprintf(outPtr, sizeof(outline) - (size_t)(outPtr - outline), "F%d ", nexF);
      outPtr += chUsed;
      curfont = nexF;
      break;
      // all the characters that can't be naked in Postscript string
    case '(': case ')': case '\\':
      if(! inString){
	inString = 1;
	*outPtr++ = '(';
      }
      *outPtr++ = '\\';
      *outPtr++ = result[i];
      break;
    case '_':  // left arrow assignment
    case '^':  // up arrow
      if(inString) {
	chUsed = snprintf(outPtr, sizeof(outline) - (size_t)(outPtr - outline), ")show ");
	outPtr += chUsed;
      }
      chUsed = snprintf(outPtr, sizeof(outline) - (size_t)(outPtr - outline), 
			"FS(\\%o)show F%d ",
			result[i] == '_' ? 0254 : 0255, curfont);
      outPtr += chUsed;
      if(inString) { 
	*outPtr++ = '(';
      }
      break;
    case 0: // end of line
      break; // so let after loop code cleanup
    default:
      if(! inString){
	inString = 1;
	*outPtr++ = '(';
      }
      *outPtr++ = result[i];
    }
  }
  if(inString)	{
    chUsed = snprintf(outPtr, sizeof(outline) - (size_t)(outPtr - outline), ")show ");
    outPtr += chUsed;
    inString = 0;
  }
  if(outPtr > outline) fprintf(out, "%s\n", outline);
  if(curY > 48) goto newline;
    fprintf(out, "\nshowpage\n");
  goto newpage;
}
