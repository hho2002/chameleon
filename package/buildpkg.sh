#!/bin/bash

# $1 Path to store built package

packagesidentity="org.chameleon"

packagename="Chameleon"

pkgroot="${0%/*}"

# blackosx to use /usr/local as a place for temporary files
# taken from http://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard#cite_note-27
chamTemp="usr/local/chamTemp"

COL_BLACK="\x1b[30;01m"
COL_RED="\x1b[31;01m"
COL_GREEN="\x1b[32;01m"
COL_YELLOW="\x1b[33;01m"
COL_MAGENTA="\x1b[35;01m"
COL_CYAN="\x1b[36;01m"
COL_WHITE="\x1b[37;01m"
COL_BLUE="\x1b[34;01m"
COL_RESET="\x1b[39;49;00m"

#version=$( grep I386BOOT_CHAMELEONVERSION vers.h | awk '{ print $3 }' | tr -d '\"' )
version=$( cat version )
stage=${version##*-}" (blackosx branch)"
revision=$( grep I386BOOT_CHAMELEONREVISION vers.h | awk '{ print $3 }' | tr -d '\"' )
builddate=$( grep I386BOOT_BUILDDATE vers.h | awk '{ print $3,$4 }' | tr -d '\"' )
timestamp=$( date -j -f "%Y-%m-%d %H:%M:%S" "${builddate}" "+%s" )

# =================

develop=$(awk "NR==6{print;exit}" ${pkgroot}/../CREDITS)
credits=$(awk "NR==10{print;exit}" ${pkgroot}/../CREDITS)
pkgdev=$(awk "NR==14{print;exit}" ${pkgroot}/../CREDITS)

# =================

distributioncount=0
xmlindent=0

indent[0]="\t"
indent[1]="\t\t"
indent[2]="\t\t\t"
indent[3]="\t\t\t\t"

main ()
{

# clean up the destination path

rm -R -f "${1}"
echo ""	
echo -e $COL_CYAN"	----------------------------------"$COL_RESET
echo -e $COL_CYAN"	Building $packagename Install Package"$COL_RESET
echo -e $COL_CYAN"	----------------------------------"$COL_RESET
echo ""

outline[$((outlinecount++))]="${indent[$xmlindent]}<choices-outline>"

# build core package
	echo "================= Core ================="
	((xmlindent++))
	packagesidentity="org.chameleon.core"
	mkdir -p ${1}/Core/Root/usr/local/bin
	mkdir -p ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot0 ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot0md ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot1f32 ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot1h ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot1he ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/boot1hp ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/cdboot ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/chain0 ${1}/Core/Root/usr/standalone/i386
	ditto --noextattr --noqtn ${1%/*}/i386/fdisk440 ${1}/Core/Root/usr/local/bin
	ditto --noextattr --noqtn ${1%/*}/i386/bdmesg ${1}/Core/Root/usr/local/bin
	local coresize=$( du -hkc "${1}/Core/Root" | tail -n1 | awk {'print $1'} )
	echo "	[BUILD] i386 "
	buildpackage "${1}/Core" "/" "0" "start_visible=\"false\" start_selected=\"true\"" >/dev/null 2>&1

# build Chameleon package
	echo "================= Chameleon ================="
	outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"Chameleon\">"
	choices[$((choicescount++))]="<choice\n\tid=\"Chameleon\"\n\ttitle=\"Chameleon_title\"\n\tdescription=\"Chameleon_description\"\n>\n</choice>\n"

	# build standard package 
		mkdir -p ${1}/Standard/Root
		mkdir -p ${1}/Standard/Scripts/Resources
		cp -f ${pkgroot}/Scripts/Main/Standard/* ${1}/Standard/Scripts
		cp -f ${pkgroot}/Scripts/Sub/* ${1}/Standard/Scripts
		ditto --arch i386 `which SetFile` ${1}/Standard/Scripts/Resources/SetFile
		ditto --noextattr --noqtn ${1%/*/*}/revision ${1}/Standard/Scripts/Resources/revision
		ditto --noextattr --noqtn ${1%/*/*}/version ${1}/Standard/Scripts/Resources/version
		echo "	[BUILD] Standard "
        buildpackage "${1}/Standard" "/" "${coresize}" "start_enabled=\"true\" start_selected=\"upgrade_allowed()\" selected=\"exclusive(choices['EFI']) &amp;&amp; exclusive(choices['noboot'])\"" >/dev/null 2>&1
	# End build standard package 

	# build efi package 
		mkdir -p ${1}/EFI/Root
		mkdir -p ${1}/EFI/Scripts/Resources
		cp -f ${pkgroot}/Scripts/Main/EFI/* ${1}/EFI/Scripts
		cp -f ${pkgroot}/Scripts/Sub/* ${1}/EFI/Scripts
		ditto --arch i386 `which SetFile` ${1}/EFI/Scripts/Resources/SetFile
		ditto --noextattr --noqtn ${1%/*/*}/revision ${1}/EFI/Scripts/Resources/revision
		ditto --noextattr --noqtn ${1%/*/*}/version ${1}/EFI/Scripts/Resources/version
		echo "	[BUILD] EFI "
		buildpackage "${1}/EFI" "/" "${coresize}" "start_visible=\"systemHasGPT()\" start_selected=\"false\" selected=\"exclusive(choices['Standard']) &amp;&amp; exclusive(choices['noboot'])\"" >/dev/null 2>&1
	# End build efi package

	# build reset choice package 
		mkdir -p ${1}/noboot/Root
		echo "	[BUILD] Reset choice "
		buildpackage "${1}/noboot" "/$chamTemp" "" "start_visible=\"true\" start_selected=\"false\" selected=\"exclusive(choices['Standard']) &amp;&amp; exclusive(choices['EFI'])\"" >/dev/null 2>&1
	# End build reset choice package 

    ((xmlindent--))
    outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"
# End build Chameleon package

# build Modules package
	echo "================= Modules ================="
	###############################
	# Supported Modules           #
	###############################
	# klibc.dylib                 #
	# Resolution.dylib            #
	# uClibcxx.dylib              #
	# Keylayout.dylib             #
	###############################
	if [ "$(ls -A "${1%/*}/i386/modules")" ]; then
	{
		outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"Module\">"
		choices[$((choicescount++))]="<choice\n\tid=\"Module\"\n\ttitle=\"Module_title\"\n\tdescription=\"Module_description\"\n>\n</choice>\n"
		((xmlindent++))
		packagesidentity="org.chameleon.modules"
# -
		if [ -e ${1%/*}/i386/modules/klibc.dylib ]; then
		{
			mkdir -p ${1}/klibc/Root
			ditto --noextattr --noqtn ${1%/*}/i386/modules/klibc.dylib ${1}/klibc/Root
			echo "	[BUILD] klibc "
			buildpackage "${1}/klibc" "/$chamTemp/Extra/modules" "" "start_selected=\"false\"" >/dev/null 2>&1
		}
		fi
# -
		if [ -e ${1%/*}/i386/modules/Resolution.dylib ]; then
		{
			mkdir -p ${1}/AutoReso/Root
			ditto --noextattr --noqtn ${1%/*}/i386/modules/Resolution.dylib ${1}/AutoReso/Root
			echo "	[BUILD] Resolution "
			buildpackage "${1}/AutoReso" "/$chamTemp/Extra/modules" "" "start_selected=\"false\"" >/dev/null 2>&1
		}
		fi
# -
		if [ -e ${1%/*}/i386/modules/uClibcxx.dylib ]; then
		{
			mkdir -p ${1}/uClibc/Root
			ditto --noextattr --noqtn ${1%/*}/i386/modules/uClibcxx.dylib ${1}/uClibc/Root
			ditto --noextattr --noqtn ${1%/*}/i386/modules/klibc.dylib ${1}/uClibc/Root
			echo "	[BUILD] uClibc++ "
			buildpackage "${1}/uClibc" "/$chamTemp/Extra/modules" "" "start_selected=\"false\"" >/dev/null 2>&1
		}
		fi
# -
		if [ -e ${1%/*}/i386/modules/Keylayout.dylib ]; then
		{
			mkdir -p ${1}/Keylayout/Root
			ditto --noextattr --noqtn ${1%/*}/i386/modules/Keylayout.dylib ${1}/Keylayout/Root
			echo "	[BUILD] Keylayout "
			buildpackage "${1}/Keylayout" "/$chamTemp/Extra/modules" "" "start_selected=\"false\"" >/dev/null 2>&1
		}
		fi

		((xmlindent--))
		outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"
	}
	else
	{
		echo "      -= no modules to include =-"
	}
	fi
# End build Modules packages


# build Extras package
	# build options packages

		outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"Options\">"
		choices[$((choicescount++))]="<choice\n\tid=\"Options\"\n\ttitle=\"Options_title\"\n\tdescription=\"Options_description\"\n>\n</choice>\n"
		((xmlindent++))

		# ------------------------------------------------------
		# parse OptionalSettings folder to find files of boot options.
		# ------------------------------------------------------
		OptionalSettingsFolder="${pkgroot}/OptionalSettings"
		OptionalSettingsFiles=($( find "${OptionalSettingsFolder}" -depth 1 ! -name '.svn' ! -name '.DS_Store' ))

		for (( i = 0 ; i < ${#OptionalSettingsFiles[@]} ; i++ ))
		do

			# Take filename and Strip .txt from end and path from front
			builtOptionsList=$( echo ${OptionalSettingsFiles[$i]%.txt} )
			builtOptionsList=$( echo ${builtOptionsList##*/} )
			echo "================= $builtOptionsList ================="
			outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"${builtOptionsList}\">"
			choices[$((choicescount++))]="<choice\n\tid=\"${builtOptionsList}\"\n\ttitle=\"${builtOptionsList}_title\"\n\tdescription=\"${builtOptionsList}_description\"\n>\n</choice>\n"
			((xmlindent++))
			packagesidentity="org.chameleon.options.$builtOptionsList"

			# ------------------------------------------------------
			# Read boot option file in to an array.
			# ------------------------------------------------------ 
			availableOptions=() # array to hold the list of boot options, per 'section'.
			exclusiveFlag=0 # used to indicate list has exclusive options.
			exclusiveName="" # will be appended to exclusive 'none' option name.
			count=0 # used as index for stepping through array.
			while read textLine
			do
				# ignore lines in the file beginning with a # and Exclusive=False
				if [[ ${textLine} != \#* ]] && [[ ${textLine} != "Exclusive=False" ]];then
					# check for 'Exclusive=True' option in file
					if [[ ${textLine} == "Exclusive=True" ]];then
						exclusiveFlag=1
						exclusiveName=$builtOptionsList
					else
						availableOptions[count]=$textLine
						((count++))
					fi
				fi
			done < ${OptionalSettingsFiles[$i]}
			buildoptionalsettings "$1" "${exclusiveFlag}" "${exclusiveName}"
			
			((xmlindent--))
			outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"
		done

		# build KeyLayout options packages
			echo "================= Keymaps Options ================="
			outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"KeyLayout\">"
			choices[$((choicescount++))]="<choice\n\tid=\"KeyLayout\"\n\ttitle=\"KeyLayout_title\"\n\tdescription=\"KeyLayout_description\"\n>\n</choice>\n"
			((xmlindent++))
			packagesidentity="org.chameleon.options.keylayout"
			
			# ------------------------------------------------------
			# Available Keylayout boot options are discovered by
			# reading contents of /Keymaps folder after compilation
			# ------------------------------------------------------
			availableOptions=()
			availableOptions=($( find "${1%/sym/*}/Keymaps" -type f -depth 1 -name '*.lyt' | sed 's|.*/||;s|\.lyt||' ))
			# Adjust array contents to match expected format
			# for boot options which is: name:key=value
			for (( i = 0 ; i < ${#availableOptions[@]} ; i++ )) 
			do
				availableOptions[i]=${availableOptions[i]}":KeyLayout="${availableOptions[i]}
			done
			
			# call buildoptionalsettings with 1 to indicate exclusive option wanted.
			buildoptionalsettings "$1" "1" "keylayout"
			
			((xmlindent--))
			outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"

		# End build KeyLayout options packages

		((xmlindent--))
		outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"

	# End build options packages

	# build theme packages
		echo "================= Themes ================="
		outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"Themes\">"
		choices[$((choicescount++))]="<choice\n\tid=\"Themes\"\n\ttitle=\"Themes_title\"\n\tdescription=\"Themes_description\"\n>\n</choice>\n"
		((xmlindent++))

		# Using themes section from Azi's/package branch.
		packagesidentity="org.chameleon.themes"
		artwork="${1%/sym/package}/artwork/themes"
		themes=($( find "${artwork}" -type d -depth 1 -not -name '.svn' ))
		for (( i = 0 ; i < ${#themes[@]} ; i++ )) 
		do
			theme=$( echo ${themes[$i]##*/} | awk 'BEGIN{OFS=FS=""}{$1=toupper($1);print}' )
			mkdir -p "${1}/${theme}/Root/"
			rsync -r --exclude=.svn "${themes[$i]}/" "${1}/${theme}/Root/${theme}"
			echo "	[BUILD] ${theme}"
			buildpackage "${1}/${theme}" "/$chamTemp/Extra/Themes" "" "start_selected=\"false\"" >/dev/null 2>&1
		done

		((xmlindent--))
		outline[$((outlinecount++))]="${indent[$xmlindent]}\t</line>"
	# End build theme packages
# End build Extras package

# build post install package
	echo "================= Post ================="
	mkdir -p ${1}/Post/Root
	mkdir -p ${1}/Post/Scripts
	cp -f ${pkgroot}/Scripts/Main/Post/* ${1}/Post/Scripts
	cp -f ${pkgroot}/Scripts/Sub/InstallLog.sh ${1}/Post/Scripts
	ditto --noextattr --noqtn ${1%/*/*}/revision ${1}/Post/Scripts/Resources/revision
	ditto --noextattr --noqtn ${1%/*/*}/version ${1}/Post/Scripts/Resources/version
	echo "	[BUILD] Post "
	buildpackage "${1}/Post" "/" "" "start_visible=\"false\" start_selected=\"true\"" >/dev/null 2>&1
	outline[$((outlinecount++))]="${indent[$xmlindent]}</choices-outline>"

# build meta package

	makedistribution "${1}" "${2}" "${3}" "${4}" "${5}"

# clean up 

	rm -R -f "${1}"

}

fixperms ()
{
	# $1 path
	find "${1}" -type f -exec chmod 644 {} \;
	find "${1}" -type d -exec chmod 755 {} \;
	chown -R 0:0 "${1}"
}

buildoptionalsettings()
{
	# $1 Path to package to build containing Root and or Scripts
	# $2 = exclusiveFlag
	# S3 = exclusiveName 
	
	# ------------------------------------------------------
	# if exclusiveFlag=1 then re-build array
	# adding extra boot option at beginning to give
	#�user a chance to choose none of them.
	# ------------------------------------------------------
	if [ ${2} = "1" ]; then
		tempArray=("${availableOptions[@]}")
		availableOptions=()
		availableOptions[0]="ChooseNone-"$3":DONT=ADD"
		position=0
		totalItems="${#tempArray[@]}"	
		for (( position = 0 ; position < $totalItems ; position++ ))
		do
			availableOptions[$position+1]=${tempArray[${position}]}
		done
	fi
	
	# ------------------------------------------------------
	# Loop through options in array and process each in turn
	# ------------------------------------------------------
	for (( c = 0 ; c < ${#availableOptions[@]} ; c++ ))
	do
		textLine=${availableOptions[c]}
		# split line - taking all before ':' as option name
		# and all after ':' as key/value
		optionName=${textLine%:*}
		keyValue=${textLine##*:}

		# create folders required for each boot option
		mkdir -p "${1}/$optionName/Root/"

		# create dummy file with name of key/value
		echo "" > "${1}/$optionName/Root/${keyValue}"

		echo "	[BUILD] ${optionName} "

		# ------------------------------------------------------
		# Before calling buildpackage, add exclusive options
		# to buildpackage call if requested.
		# ------------------------------------------------------
		if [ $2 = "1" ]; then

			# Prepare individual string parts
			stringStart="selected=\""
			stringBefore="exclusive(choices['"
			stringAfter="']) &amp;&amp; "
			stringEnd="'])\""
			x=${stringStart}${stringBefore}

			# build string for sending to buildpackage
			totalItems="${#availableOptions[@]}"
			lastItem=$((totalItems-1))

			for (( r = 0 ; r < ${totalItems} ; r++ ))
			do
				textLineTemp=${availableOptions[r]}
				optionNameTemp=${textLineTemp%:*}
				if [ "${optionNameTemp}" != "${optionName}" ]; then
					 x="${x}${optionNameTemp}"
					 # Only add these to end of string up to the one before the last item
					if [ $r -lt $lastItem ]; then
						x="${x}${stringAfter}${stringBefore}"
					fi
				fi
			done
			x="${x}${stringEnd}"
			buildpackage "${1}/${optionName}" "/$chamTemp/options" "" "start_selected=\"false\" ${x}" >/dev/null 2>&1
		else
			buildpackage "${1}/${optionName}" "/$chamTemp/options" "" "start_selected=\"false\"" >/dev/null 2>&1
		fi
	done
}

buildpackage ()
{
#  $1 Path to package to build containing Root and or Scripts
#  $2 Install Location
#  $3 Size
#  $4 Options

if [ -d "${1}/Root" ] && [ "${1}/Scripts" ]; then

	local packagename="${1##*/}"
	local identifier=$( echo ${packagesidentity}.${packagename//_/.} | tr [:upper:] [:lower:] )
	find "${1}" -name '.DS_Store' -delete
	local filecount=$( find "${1}/Root" | wc -l )
	if [ "${3}" ]; then
		local installedsize="${3}"
	else
		local installedsize=$( du -hkc "${1}/Root" | tail -n1 | awk {'print $1'} )
	fi
	local header="<?xml version=\"1.0\"?>\n<pkg-info format-version=\"2\" "

	#[ "${3}" == "relocatable" ] && header+="relocatable=\"true\" "		

	header+="identifier=\"${identifier}\" "
	header+="version=\"${version}\" "

	[ "${2}" != "relocatable" ] && header+="install-location=\"${2}\" "

	header+="auth=\"root\">\n"
	header+="\t<payload installKBytes=\"${installedsize##* }\" numberOfFiles=\"${filecount##* }\"/>\n"
	rm -R -f "${1}/Temp"

	[ -d "${1}/Temp" ] || mkdir -m 777 "${1}/Temp"
	[ -d "${1}/Root" ] && mkbom "${1}/Root" "${1}/Temp/Bom"

	if [ -d "${1}/Scripts" ]; then 
		header+="\t<scripts>\n"
		for script in $( find "${1}/Scripts" -type f \( -name 'pre*' -or -name 'post*' \) )
		do
			header+="\t\t<${script##*/} file=\"./${script##*/}\"/>\n"
		done
		header+="\t</scripts>\n"
		chown -R 0:0 "${1}/Scripts"
		pushd "${1}/Scripts" >/dev/null
		find . -print | cpio -o -z -H cpio > "../Temp/Scripts"
		popd >/dev/null
	fi

	header+="</pkg-info>"
	echo -e "${header}" > "${1}/Temp/PackageInfo"
	pushd "${1}/Root" >/dev/null
	find . -print | cpio -o -z -H cpio > "../Temp/Payload"
	popd >/dev/null
	pushd "${1}/Temp" >/dev/null

	xar -c -f "${1%/*}/${packagename// /}.pkg" --compression none .

	popd >/dev/null

	outline[$((outlinecount++))]="${indent[$xmlindent]}\t<line choice=\"${packagename// /}\"/>"

	if [ "${4}" ]; then
		local choiceoptions="${indent[$xmlindent]}${4}\n"	
	fi
	choices[$((choicescount++))]="<choice\n\tid=\"${packagename// /}\"\n\ttitle=\"${packagename}_title\"\n\tdescription=\"${packagename}_description\"\n${choiceoptions}>\n\t<pkg-ref id=\"${identifier}\" installKBytes='${installedsize}' version='${version}.0.0.${timestamp}' auth='root'>#${packagename// /}.pkg</pkg-ref>\n</choice>\n"

	rm -R -f "${1}"
fi
}

makedistribution ()
{
	rm -f "${1%/*}/${packagename// /}"*.pkg

	find "${1}" -type f -name '*.pkg' -depth 1 | while read component
	do
		mkdir -p "${1}/${packagename}/${component##*/}"
		pushd "${1}/${packagename}/${component##*/}" >/dev/null
		xar -x -f "${1%}/${component##*/}"
		popd >/dev/null
	done

	ditto --noextattr --noqtn "${pkgroot}/Distribution" "${1}/${packagename}/Distribution"
	ditto --noextattr --noqtn "${pkgroot}/Resources" "${1}/${packagename}/Resources"

	find "${1}/${packagename}/Resources" -type d -name '.svn' -exec rm -R -f {} \; 2>/dev/null

	for (( i=0; i < ${#outline[*]} ; i++));
		do
			echo -e "${outline[$i]}" >> "${1}/${packagename}/Distribution"
		done

	for (( i=0; i < ${#choices[*]} ; i++));
		do
			echo -e "${choices[$i]}" >> "${1}/${packagename}/Distribution"
		done

	echo "</installer-gui-script>"  >> "${1}/${packagename}/Distribution"

	perl -i -p -e "s/%CHAMELEONVERSION%/${version%%-*}/g" `find "${1}/${packagename}/Resources" -type f`
	perl -i -p -e "s/%CHAMELEONREVISION%/${revision}/g" `find "${1}/${packagename}/Resources" -type f`

#  Adding Developer and credits
	perl -i -p -e "s/%DEVELOP%/${develop}/g" `find "${1}/${packagename}/Resources" -type f`
	perl -i -p -e "s/%CREDITS%/${credits}/g" `find "${1}/${packagename}/Resources" -type f`
	perl -i -p -e "s/%PKGDEV%/${pkgdev}/g" `find "${1}/${packagename}/Resources" -type f`

	stage=${stage/RC/Release Candidate }
	stage=${stage/FINAL/2.0 Final}
	perl -i -p -e "s/%CHAMELEONSTAGE%/${stage}/g" `find "${1}/${packagename}/Resources" -type f`

	find "${1}/${packagename}" -name '.DS_Store' -delete
	pushd "${1}/${packagename}" >/dev/null
	xar -c -f "${1%/*}/${packagename// /}-${version}-r${revision}.pkg" --compression none .
	popd >/dev/null

#   Here is the place for assign a Icon to the pkg
#   command use to generate the file:
#   ditto -c -k --sequesterRsrc --keepParent Icon.icns Icon.zip
# ----
#    ditto -xk "${pkgroot}/Icons/pkg.zip" "${pkgroot}/Icons/"
#    DeRez -only icns "${pkgroot}/Icons/Icons/pkg.icns" > tempicns.rsrc
#    Rez -append tempicns.rsrc -o "${1%/*}/$packagename-${version}-r$revision.pkg"
#    SetFile -a C "${1%/*}/$packagename-${version}-r$revision.pkg"
#    rm -f tempicns.rsrc
#    rm -rf "${pkgroot}/Icons/Icons"
# End

	md5=$( md5 "${1%/*}/${packagename// /}-${version}-r${revision}.pkg" | awk {'print $4'} )
	echo "MD5 (${packagename// /}-${version}-r${revision}.pkg) = ${md5}" > "${1%/*}/${packagename// /}-${version}-r${revision}.pkg.md5"
	echo ""	

	echo -e $COL_GREEN"	--------------------------"$COL_RESET
	echo -e $COL_GREEN"	Building process complete!"$COL_RESET
	echo -e $COL_GREEN"	--------------------------"$COL_RESET
	echo ""	
	echo -e $COL_GREEN"	Build info."
	echo -e $COL_GREEN"	==========="
	echo -e $COL_BLUE"	Package name:	"$COL_RESET"$packagename-${version}-r$revision.pkg"
	echo -e $COL_BLUE"	MD5:		"$COL_RESET"$md5"
	echo -e $COL_BLUE"	Version:	"$COL_RESET"$version"
	echo -e $COL_BLUE"	Stage:		"$COL_RESET"$stage"
	echo -e $COL_BLUE"	Date/Time:	"$COL_RESET"$builddate"
	echo ""

}

main "${1}" "${2}" "${3}" "${4}" "${5}"

