# #%L
# %%
# Copyright (C) 2011 - 2017 BMW Car IT GmbH
# %%
# __________________
# 
# NOTICE:  Dissemination of this information or reproduction of this material 
# is strictly  forbidden unless prior written permission is obtained from 
# BMW Car IT GmbH.
# #L%


function(remove_include_prefix file_name prefixes result_var)
#	message("[DEBUG] entering remove_include_prefix")
#	message("[DEBUG] file_name=${file_name}")
#	message("[DEBUG] prefixes=${prefixes}")
#	message("[DEBUG] include_name=${include_name}")
	set(prefixes_filtered)

	foreach(d ${prefixes})
#		message("[DEBUG] d=${d}")
		# remove trailing slash from the directory name
		string(REGEX REPLACE "/$" "" dd ${d})
		# if it is not the current directory 
		if(NOT (${dd} STREQUAL "."))
			# add it to the prefix list
			set(prefixes_filtered ${prefixes_filtered} ${dd})
		endif(NOT (${dd} STREQUAL "."))
	endforeach(d)

	# find the longest matching prefix
	set(longest_match_length 0)
	foreach(prefix ${prefixes_filtered})
		# if current prefix matches
		if(${file_name} MATCHES "^${prefix}/")
			# get the current match
			string(REGEX MATCH "^${prefix}" match ${file_name})
			# calculate length of current match
			string(LENGTH ${match} current_match_length)
			# if longer match is found
			if(${current_match_length} GREATER ${longest_match_length})
				# update longest match
				set(longest_match_length ${current_match_length})
			endif(${current_match_length} GREATER ${longest_match_length})
		endif(${file_name} MATCHES "^${prefix}/")
	endforeach(prefix)
	# get the longest matching prefix
	string(SUBSTRING ${file_name} 0 ${longest_match_length} longest_matching_prefix)
	# remove the longest matching prefix
	string(REGEX REPLACE "^${longest_matching_prefix}/" "" include_name ${file_name})
	set(${result_var} ${include_name} PARENT_SCOPE)
#	message("[DEBUG] the include name for ${file_name} is ${include_name}")
endfunction(remove_include_prefix)

function(add_custom_build_cmd_to_copy_headers_to_global_include_dir target_name include_dir_postfix include_dirs)
foreach(header_file ${ARGN})
	remove_include_prefix(${header_file} "${include_dirs}" include_name)
#   message(STATUS "processing header_file \"${header_file}\" -> include name \"${include_name}\"")
	set(
		GLOBAL_INCLUDE_DIR
		${JOYNR_INCLUDE_DIR}
	)
#	message("[DEBUG] include_dir_postfix=${include_dir_postfix}")
	if(NOT ${include_dir_postfix})
		set(
			GLOBAL_INCLUDE_DIR
			"${GLOBAL_INCLUDE_DIR}/${include_dir_postfix}"
		)
	endif(NOT ${include_dir_postfix})
#	message("[DEBUG] GLOBAL_INCLUDE_DIR=${GLOBAL_INCLUDE_DIR}")

	add_custom_command(
		TARGET ${target_name}
		POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/${header_file} 
			${GLOBAL_INCLUDE_DIR}/${include_name}
	)
endforeach(header_file)
endfunction(add_custom_build_cmd_to_copy_headers_to_global_include_dir)

function(copy_headers_to_global_include_dir include_dir_postfix include_dirs)
foreach(header_file ${ARGN})
	remove_include_prefix(${header_file} "${include_dirs}" include_name)
#   message(STATUS "processing header_file \"${header_file}\" -> include name \"${include_name}\"")
	set(
		GLOBAL_INCLUDE_DIR
		${JOYNR_INCLUDE_DIR}
	)
#	message("[DEBUG] include_dir_postfix=${include_dir_postfix}")
	if(NOT ${include_dir_postfix})
		set(
			GLOBAL_INCLUDE_DIR
			"${GLOBAL_INCLUDE_DIR}/${include_dir_postfix}"
		)
	endif(NOT ${include_dir_postfix})
#	message("[DEBUG] GLOBAL_INCLUDE_DIR=${GLOBAL_INCLUDE_DIR}")

	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/${header_file}
		${GLOBAL_INCLUDE_DIR}/${include_name}
		COPYONLY
	)
endforeach(header_file)
endfunction(copy_headers_to_global_include_dir)

