/*
 * ESG parser
 *
 * Copyright (C) 2006 Stephane Este-Gracias (sestegra@free.fr)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdlib.h>
#include <string.h>

#include <libesg/transport/session_partition_declaration.h>

struct esg_session_partition_declaration *esg_session_partition_declaration_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_session_partition_declaration *partition;
	struct esg_session_field *field;
	struct esg_session_field *last_field;
	uint8_t field_index;
	struct esg_session_ip_stream *ip_stream;
	struct esg_session_ip_stream *last_ip_stream;
	uint8_t ip_stream_index;
	uint8_t ip_index;
	struct esg_session_ip_stream_field *ip_stream_field;
	struct esg_session_ip_stream_field *last_ip_stream_field;
	uint8_t *field_buffer;
	uint32_t field_length;
	union esg_session_ip_stream_field_value *field_value;

	if ((buffer == NULL) || (size <= 2)) {
		return NULL;
	}

	pos = 0;

	partition = (struct esg_session_partition_declaration *) malloc(sizeof(struct esg_session_partition_declaration));
	memset(partition, 0, sizeof(struct esg_session_partition_declaration));
	partition->field_list = NULL;
	partition->ip_stream_list = NULL;

	partition->num_fields = buffer[pos];
	pos += 1;

	partition->overlapping = (buffer[pos] & 0x80) ? 1 : 0;
	pos += 1;

	if (size < (pos + 5*(partition->num_fields))) {
		esg_session_partition_declaration_free(partition);
		return NULL;
	}

	last_field = NULL;
	for (field_index = 0; field_index < partition->num_fields; field_index++) {
		field = (struct esg_session_field *) malloc(sizeof(struct esg_session_field));
		memset(field, 0, sizeof(struct esg_session_field));
		field->_next = NULL;

		if (last_field == NULL) {
			partition->field_list = field;
		} else {
			last_field->_next = field;
		}
		last_field = field;

		field->identifier = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		field->encoding = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		field->length = buffer[pos];
		pos += 1;
	}

	partition->n_o_ip_streams = buffer[pos];
	pos += 1;

	partition->ip_version_6 = (buffer[pos] & 0x80) ? 1 : 0;
	pos += 1;

	last_ip_stream = NULL;
	for (ip_stream_index = 0; ip_stream_index < partition->n_o_ip_streams; ip_stream_index++) {
		ip_stream = (struct esg_session_ip_stream *) malloc(sizeof(struct esg_session_ip_stream));
		memset(ip_stream, 0, sizeof(struct esg_session_ip_stream));
		ip_stream->_next = NULL;

		if (last_ip_stream == NULL) {
			partition->ip_stream_list = ip_stream;
		} else {
			last_ip_stream->_next = ip_stream;
		}
		last_ip_stream = ip_stream;

		ip_stream->id = buffer[pos];
		pos += 1;

		if (partition->ip_version_6) {
			for (ip_index = 0; ip_index < 16; ip_index++) {
				ip_stream->source_ip.ipv6[ip_index] = buffer[pos+ip_index];
				ip_stream->destination_ip.ipv6[ip_index] = buffer[pos+16+ip_index];
			}
			pos += 32;
		} else {
			for (ip_index = 0; ip_index < 4; ip_index++) {
				ip_stream->source_ip.ipv4[ip_index] = buffer[pos+ip_index];
				ip_stream->destination_ip.ipv4[ip_index] = buffer[pos+4+ip_index];
			}
			pos += 8;
		}
		ip_stream->port = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		ip_stream->session_id = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		last_ip_stream_field = NULL;
		esg_session_partition_declaration_field_list_for_each(partition, field) {
			ip_stream_field = (struct esg_session_ip_stream_field *) malloc(sizeof(struct esg_session_ip_stream_field));
			memset(ip_stream_field, 0, sizeof(struct esg_session_ip_stream_field));
			ip_stream_field->_next = NULL;
			ip_stream_field->start_field_value = NULL;
			ip_stream_field->end_field_value = NULL;

			if (last_ip_stream_field == NULL) {
				ip_stream->field_list = ip_stream_field;
			} else {
				last_ip_stream_field->_next = ip_stream_field;
			}
			last_ip_stream_field = ip_stream_field;

			field_length = field->length;
			if (field->length != 0) {
				field_length = field->length;
			} else {
				pos += vluimsbf8(buffer + pos, size - pos, &field_length);
			}

			switch (field->encoding) {
				case 0x0000: {
					if (partition->overlapping == 1) {
						field_value = (union esg_session_ip_stream_field_value *) malloc(sizeof(union esg_session_ip_stream_field_value));
						memset(field_value, 0, sizeof(union esg_session_ip_stream_field_value));
						ip_stream_field->start_field_value = field_value;

						field_buffer = (uint8_t *) malloc(field_length);
						memset(field_buffer, 0, field_length);
						memcpy(field_buffer, buffer + pos, field_length);

						ip_stream_field->start_field_value->string = field_buffer;
						pos += field_length;
					}
					field_value = (union esg_session_ip_stream_field_value *) malloc(sizeof(union esg_session_ip_stream_field_value));
					memset(field_value, 0, sizeof(union esg_session_ip_stream_field_value));
					ip_stream_field->end_field_value = field_value;

					field_buffer = (uint8_t *) malloc(field_length);
					memset(field_buffer, 0, field_length);
					memcpy(field_buffer, buffer + pos, field_length);

					ip_stream_field->end_field_value->string = field_buffer;
					pos += field_length;

					break;
				}
				case 0x0101: {
					if (partition->overlapping == 1) {
						field_value = (union esg_session_ip_stream_field_value *) malloc(sizeof(union esg_session_ip_stream_field_value));
						memset(field_value, 0, sizeof(union esg_session_ip_stream_field_value));
						ip_stream_field->start_field_value = field_value;

						ip_stream_field->start_field_value->unsigned_short = (buffer[pos] << 8) | buffer[pos+1];
						pos += field_length;
					}
					field_value = (union esg_session_ip_stream_field_value *) malloc(sizeof(union esg_session_ip_stream_field_value));
					memset(field_value, 0, sizeof(union esg_session_ip_stream_field_value));
					ip_stream_field->end_field_value = field_value;

					ip_stream_field->end_field_value->unsigned_short = (buffer[pos] << 8) | buffer[pos+1];
					pos += field_length;

					break;
				}
				default: {
					esg_session_partition_declaration_free(partition);
					return NULL;
				}
			}
		}
	}

	return partition;
}

void esg_session_partition_declaration_free(struct esg_session_partition_declaration *partition) {
	struct esg_session_field *field;
	struct esg_session_field *next_field;
	struct esg_session_ip_stream *ip_stream;
	struct esg_session_ip_stream *next_ip_stream;
	struct esg_session_ip_stream_field *ip_stream_field;
	struct esg_session_ip_stream_field *next_ip_stream_field;

	if (partition == NULL) {
		return;
    }

	for(ip_stream = partition->ip_stream_list; ip_stream; ip_stream = next_ip_stream) {
		next_ip_stream = ip_stream->_next;

		field = partition->field_list;
		for(ip_stream_field = next_ip_stream->field_list; ip_stream_field; ip_stream_field = next_ip_stream_field) {
			next_ip_stream_field = ip_stream_field->_next;

			switch (field->encoding) {
				case 0x0000: {
					if (ip_stream_field->start_field_value != NULL) {
						free(ip_stream_field->start_field_value->string);
					}
					free(ip_stream_field->end_field_value->string);
					break;
				}
				case 0x0101: {
					// Nothing to free
					break;
				}
			}

			free(ip_stream_field);

			field = field->_next;
		}

		free(ip_stream);
	}

	for(field = partition->field_list; field; field = next_field) {
		next_field = field->_next;
		free(field);
	}

	free(partition);
}
