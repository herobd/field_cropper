This project relies on Doug Kennard's "documentproject" code. This directory is to be placed in the "apps" directory for the Makefile to work.

Usage:
`field_cropper outputdir pointerfile rootDir templatefile (startPageId)`

`outputdir`: The root dir it will create the field dirs in.

`pointerfile`: A file containing "transcription-x.xml image-x.jpg" on each line. This is the FamilySearch IRIS xml and its corresponding image.

`rootDir`: The path prepended to the files in pointerfile.

`templatefile`: This is a csv file. First line is field names (e.g. "PR_NAME,PR_RELATIONSHIP,PR_SEX..."). Each following line is sets of "x1 y1 x2 y2" corresponding to each field name, giving the locations of that field for a particular entry.

`startPageId`: optionally, start running from the given page.

