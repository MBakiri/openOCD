/***************************************************************************
 *   Copyright (C) 2012 by Franck Jullien                                  *
 *   elec4fun@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"
#include "register.h"
#include "target.h"
#include "target_type.h"
#include "fileio.h"

/* Get the target registers list and write a tdesc feature section with
 * all registers matching feature_name. If feature_name is NULL, create
 * a "nogroup" feature with all registers without feature definition (""
 * or NULL).
 */
int generate_feature_section(struct target *target, struct fileio *fileio, const char *feature_name)
{
	struct reg **reg_list;
	int reg_list_size;
	int retval;
	int i;
	int nogroup = 0;
	int add_reg_to_group = 0;

	retval = target_get_gdb_reg_list(target, &reg_list, &reg_list_size, FULL_LIST);
	if (retval != ERROR_OK)
		return retval;

	if ((feature_name != NULL && !strcmp(feature_name, "")) || feature_name == NULL)
		nogroup = 1;

	if (nogroup)
		fileio_fprintf(fileio,"  <feature name=\"org.gnu.gdb.or32.%s\">\n", "nogroup");
	else
		fileio_fprintf(fileio,"  <feature name=\"org.gnu.gdb.or32.%s\">\n", feature_name);

	for (i = 0; i < reg_list_size; i++) {
		if (nogroup) {
			if ((reg_list[i]->feature != NULL && !strcmp(reg_list[i]->feature, ""))
			     || reg_list[i]->feature == NULL) {
				add_reg_to_group = 1;
			}
		} else {
			if (reg_list[i]->feature != NULL && strcmp(reg_list[i]->feature, "")) {
				if (!strcmp(reg_list[i]->feature, feature_name)) {
					add_reg_to_group = 1;
				}
			}
		}

		if (add_reg_to_group) {
			fileio_fprintf(fileio, "    <reg name=\"%s\"           bitsize=\"%d\" regnum=\"%d\"/>\n",
			reg_list[i]->name, reg_list[i]->size, i);
			add_reg_to_group = 0;
		}
	}

	fileio_fprintf(fileio, "  </feature>\n");

	free(reg_list);

	return ERROR_OK;
}

/* Get a list of available target registers features. feature_list must
 * be freed by caller.
 */
int get_reg_features_list(struct target *target, char **feature_list[])
{
	struct reg **reg_list;
	int reg_list_size;
	int retval;
	int tbl_sz = 0;
	int i,j;

	retval = target_get_gdb_reg_list(target, &reg_list, &reg_list_size, FULL_LIST);
	if (retval != ERROR_OK) {
		*feature_list = NULL;
		return retval;
	}

	/* Start with only one element */
	*feature_list = calloc(1, sizeof(char *));

	for (i = 0; i < reg_list_size; i++) {
		if (reg_list[i]->feature != NULL && strcmp(reg_list[i]->feature, "")) {
			/* We found a feature, check if the feature is already in the
			 * table. If not, add allocate a new entry for the table and
			 * put the new feature in it.
			 */
			for (j = 0; j < (tbl_sz + 1); j++) {
					if (!((*feature_list)[j])) {
						(*feature_list)[tbl_sz++] = strdup(reg_list[i]->feature);
						*feature_list = realloc(*feature_list, sizeof(char *) * (tbl_sz + 1));
						(*feature_list)[tbl_sz] = NULL;
						break;
					} else {
						if (!strcmp((*feature_list)[j], reg_list[i]->feature))
							break;
					}
			}
		}
	}

	free(reg_list);

	return tbl_sz;
}

/* Returns how many registers don't have a feature specified */
int count_reg_without_group(struct target *target)
{
	struct reg **reg_list;
	int reg_list_size;
	int retval;
	int i;
	int reg_without_group = 0;

	retval = target_get_gdb_reg_list(target, &reg_list, &reg_list_size, FULL_LIST);
	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < reg_list_size; i++) {
			if ((reg_list[i]->feature != NULL && !strcmp(reg_list[i]->feature, ""))
			     || reg_list[i]->feature == NULL) {
				reg_without_group++;
			}
	}

	free(reg_list);

	return reg_without_group;
}

/* Open a file for write, set the header of the file according to the
 * gdb target description format and configure the architecture element with
 * the given arch_name.
 */
int open_and_init_tdesc_file(struct fileio *fileio, const char *filename, const char *arch_name)
{
	int retval;

	retval = fileio_open(fileio, filename, FILEIO_WRITE, FILEIO_TEXT);
	if (retval != ERROR_OK)
		return ERROR_FAIL;

	fileio_fprintf(fileio, "<?xml version=\"1.0\"?>\n");
	fileio_fprintf(fileio, "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n");
	fileio_fprintf(fileio, "<target>\n");
	fileio_fprintf(fileio, "  <architecture>%s</architecture>\n\n", arch_name);

	return ERROR_OK;
}

/* Close a target descriptor file */
int close_tdesc_file(struct fileio *fileio)
{
	fileio_fprintf(fileio, "</target>\n");
	fileio_close(fileio);

	return ERROR_OK;
}