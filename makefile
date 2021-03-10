#--------------------------------------------------#
#                  Dirs                            #
#--------------------------------------------------#
SRCDIR		= /usr/

# Compiler flags...
ROOT_COMPILER	= ""
C_COMPILER 		= "gcc"
CPP_COMPILER 	= "g++"

# Additional include paths
Release_INC= 

# Additional librarie paths
Release_LIB= 

# Additional librarie paths 64 bits
Release_LIB64= 

# Preprocessor definitions...
Release_DEFS=-D _SSL_CERT_EMBEDDED_ -D COMM_SSL -D VERSION_OPENSSL=111 -D _LINUX -D PP_ABECS -D _RSA_PAYGO_
Release_DEFS+=-D _PIN_TREATMENT_ -D _LIBLINUX32_ -D _PIN_QRCODE_ -D _PIN_AIDEXTENDED_ -D _PW_TSTKEY_ -D _USE_POSIX_
Release64_DEFS=-D _SSL_CERT_EMBEDDED_ -D COMM_SSL -D VERSION_OPENSSL=111 -D _LINUX -D PP_ABECS -D _RSA_PAYGO_ -D RSA_STDINT_TYPES -D _USE_POSIX_
Release64_DEFS+=-D _PIN_TREATMENT_ -D_LIBLINUX64_ -D _PIN_QRCODE_ -D _PIN_AIDEXTENDED_ -D _PW_TSTKEY_
Debug_DEFS=-D _DEBUG -D _DEBUG_ 
Prod_DEFS=-D _PROD_ 

# Compiler flags...
Release_Flags=-Wall -O2  -m32
Release64_Flags=-Wall -O2 -m64 -fPIC
Debug64_Flags=-Wall -m64 -fPIC -g

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations:

###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

.PHONY: Test
Test: create_folders gccRelease/PGWebLibTest.o
	$(C_COMPILER) -m32 -o gccRelease/PGWebLibTest.exe gccRelease/PGWebLibTest.o -ldl

# Compiles file PGWebLibTest.c for the Release configuration...
-include gccRelease/PGWebLibTest.d
gccRelease/PGWebLibTest.o: PGWebLibTest.c
	$(C_COMPILER) $(Release_INC) $(Release_DEFS) $(Release_Flags) -c PGWebLibTest.c -o gccRelease/PGWebLibTest.o
	$(C_COMPILER) $(Release_INC) $(Release_DEFS) $(Release_Flags) -MM PGWebLibTest.c > gccRelease/PGWebLibTest.d



###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

# Builds the Release64 configuration...
.PHONY: Test64
Test64: create_folders64 gccRelease64/PGWebLibTest.o
	$(C_COMPILER) -m64 -o gccRelease64/PGWebLibTest.exe gccRelease64/PGWebLibTest.o -ldl

# Compiles file PGWebLibTest.c for the Release64 configuration...
-include gccRelease64/PGWebLibTest.d
gccRelease64/PGWebLibTest.o: PGWebLibTest.c
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Release64_Flags) -c PGWebLibTest.c -o gccRelease64/PGWebLibTest.o
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Release64_Flags) -MM PGWebLibTest.c > gccRelease64/PGWebLibTest.d


###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

.PHONY: TestProd
TestProd: create_foldersProd gccReleaseProd/PGWebLibTest.o
	$(C_COMPILER) -m32 -o gccReleaseProd/PGWebLibTest.exe gccReleaseProd/PGWebLibTest.o -ldl

# Compiles file PGWebLibTest.c for the ReleaseProd configuration...
-include gccReleaseProd/PGWebLibTest.d
gccReleaseProd/PGWebLibTest.o: PGWebLibTest.c
	$(C_COMPILER) $(Release_INC) $(Release_DEFS) $(Prod_DEFS) $(Release_Flags) -c PGWebLibTest.c -o gccReleaseProd/PGWebLibTest.o
	$(C_COMPILER) $(Release_INC) $(Release_DEFS) $(Prod_DEFS) $(Release_Flags) -MM PGWebLibTest.c > gccReleaseProd/PGWebLibTest.d


	

###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

# Builds the ReleaseProd64 configuration...

.PHONY: TestProd64
TestProd64: create_foldersProd64 gccReleaseProd64/PGWebLibTest.o
	$(C_COMPILER) -m64 -o gccReleaseProd64/PGWebLibTest.exe gccReleaseProd64/PGWebLibTest.o -ldl

# Compiles file PGWebLibTest.c for the ReleaseProd64 configuration...
-include gccReleaseProd64/PGWebLibTest.d
gccReleaseProd64/PGWebLibTest.o: PGWebLibTest.c
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Prod_DEFS) $(Release64_Flags) -c PGWebLibTest.c -o gccReleaseProd64/PGWebLibTest.o
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Prod_DEFS) $(Release64_Flags) -MM PGWebLibTest.c > gccReleaseProd64/PGWebLibTest.d


	
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

# Builds the Debug64 configuration...

.PHONY: TestDebug64
TestDebug64: create_foldersDebug64 gccDebug64/PGWebLibTest.o
	$(C_COMPILER) -g -m64 -o gccDebug64/PGWebLibTest.exe gccDebug64/PGWebLibTest.o -ldl

# Compiles file PGWebLibTest.c for the Debug64 configuration...
-include gccDebug64/PGWebLibTest.d
gccDebug64/PGWebLibTest.o: PGWebLibTest.c
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Debug_DEFS) $(Debug64_Flags) -c PGWebLibTest.c -o gccDebug64/PGWebLibTest.o
	$(C_COMPILER) $(Release_INC) $(Release64_DEFS) $(Debug_DEFS) $(Debug64_Flags) -MM PGWebLibTest.c > gccDebug64/PGWebLibTest.d

###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################
###########################################################################################################################################

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccRelease

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders64
create_folders64:
	mkdir -p gccRelease64

# Creates the intermediate and output folders for each configuration...
.PHONY: create_foldersProd
create_foldersProd:
	mkdir -p gccReleaseProd

# Creates the intermediate and output folders for each configuration...
.PHONY: create_foldersProd64
create_foldersProd64:
	mkdir -p gccReleaseProd64

# Creates the intermediate and output folders for each configuration...
.PHONY: create_foldersDebug64
create_foldersDebug64:
	mkdir -p gccDebug64

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccRelease/*.o
	rm -f gccRelease/*.d
	rm -f gccRelease/*.a
	rm -f gccRelease/*.so
	rm -f gccRelease/*.dll
	rm -f gccRelease/*.exe

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean64
clean64:
	rm -f gccRelease64/*.o
	rm -f gccRelease64/*.d
	rm -f gccRelease64/*.a
	rm -f gccRelease64/*.so
	rm -f gccRelease64/*.dll
	rm -f gccRelease64/*.exe

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: cleanProd
cleanProd:
	rm -f gccReleaseProd/*.o
	rm -f gccReleaseProd/*.d
	rm -f gccReleaseProd/*.a
	rm -f gccReleaseProd/*.so
	rm -f gccReleaseProd/*.dll
	rm -f gccReleaseProd/*.exe

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: cleanProd64
cleanProd64:
	rm -f gccReleaseProd64/*.o
	rm -f gccReleaseProd64/*.d
	rm -f gccReleaseProd64/*.a
	rm -f gccReleaseProd64/*.so
	rm -f gccReleaseProd64/*.dll
	rm -f gccReleaseProd64/*.exe
	
# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: cleanDebug64
cleanDebug64:
	rm -f gccDebug64/*.o
	rm -f gccDebug64/*.d
	rm -f gccDebug64/*.a
	rm -f gccDebug64/*.so
	rm -f gccDebug64/*.dll
	rm -f gccDebug64/*.exe

# Cleans ALL intermediate and output files (objects, libraries, executables)...
.PHONY: cleanAll
cleanAll: clean clean64 cleanProd cleanProd64 cleanDebug64

# Builds ALL configurations...
.PHONY: all
all: Test Test64 TestProd TestProd64 \
TestDebug64 