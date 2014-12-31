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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <libesg/bootstrap/access_descriptor.h>
#include <libesg/encapsulation/container.h>
#include <libesg/encapsulation/fragment_management_information.h>
#include <libesg/encapsulation/data_repository.h>
#include <libesg/encapsulation/string_repository.h>
#include <libesg/representation/encapsulated_textual_esg_xml_fragment.h>
#include <libesg/representation/init_message.h>
#include <libesg/representation/textual_decoder_init.h>
#include <libesg/representation/bim_decoder_init.h>
#include <libesg/transport/session_partition_declaration.h>

#define MAX_FILENAME 256

void usage(void) {
  static const char *_usage =
    "Usage: testesg [-a <ESGAccessDescriptor>]\n"
    "               [-c <ESGContainer with Textual ESG XML Fragment>]\n"
    "               [-X XXXX]\n";

  fprintf(stderr, "%s", _usage);
  exit(1);
}

void read_from_file(const char *filename, char **buffer, int *size) {
	int fd;
	struct stat fs;

	if ((fd = open(filename, O_RDONLY)) <= 0) {
		fprintf(stderr, "File not found\n");
		exit(1);
	}

	if (fstat(fd, &fs) < 0) {
		fprintf(stderr, "File not readable\n");
		exit(1);
	}
	*size = fs.st_size;

	*buffer = (char *) malloc(*size);
	if (read(fd, *buffer, *size) != *size) {
		fprintf(stderr, "File read error\n");
		exit(1);
	}

	close(fd);

	return;
}

int main(int argc, char *argv[]) {
	char access_descriptor_filename[MAX_FILENAME] = "";
	char container_filename[MAX_FILENAME] = "";
	int c;
	char *buffer = NULL;
	int size;

	// Read command line options
	while ((c = getopt(argc, argv, "a:c:")) != -1) {
		switch (c) {
			case 'a':
				strncpy(access_descriptor_filename, optarg, MAX_FILENAME);
				break;
			case 'c':
				strncpy(container_filename, optarg, MAX_FILENAME);
				break;
			default:
				usage();
		}
	}

	// ESGAccessDescriptor
	if (strncmp(access_descriptor_filename, "", MAX_FILENAME) != 0) {
		fprintf(stdout, "**************************************************\n");
		fprintf(stdout, "Reading ESG Access Descriptor = %s\n", access_descriptor_filename);
		fprintf(stdout, "**************************************************\n\n");

		read_from_file(access_descriptor_filename, &buffer, &size);

		struct esg_access_descriptor *access_descriptor = esg_access_descriptor_decode((uint8_t *) buffer, size);
		free(buffer);
		if (access_descriptor == NULL) {
			fprintf(stderr, "ESG Access Descriptor decode error\n");
			exit(1);
		}
		fprintf(stdout, "n_o_ESGEntries %d\n\n", access_descriptor->n_o_entries);

		struct esg_entry *entry;
		esg_access_descriptor_entry_list_for_each(access_descriptor, entry) {
			fprintf(stdout, "    ESGEntryVersion %d\n", entry->version);
			fprintf(stdout, "    MultipleStreamTransport %d\n", entry->multiple_stream_transport);
			fprintf(stdout, "    IPVersion6 %d\n", entry->ip_version_6);
			fprintf(stdout, "    ProviderID %d\n", entry->provider_id);
			if (entry->ip_version_6 == 0) {
				fprintf(stdout, "    SourceIPAddress %d.%d.%d.%d\n",
					entry->source_ip.ipv4[0],
					entry->source_ip.ipv4[1],
					entry->source_ip.ipv4[2],
					entry->source_ip.ipv4[3]);
				fprintf(stdout, "    DestinationIPAddress %d.%d.%d.%d\n",
					entry->destination_ip.ipv4[0],
					entry->destination_ip.ipv4[1],
					entry->destination_ip.ipv4[2],
					entry->destination_ip.ipv4[3]);
			} else if (entry->ip_version_6 == 1) {
				fprintf(stdout, "    SourceIPAddress %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
					entry->source_ip.ipv6[0],
					entry->source_ip.ipv6[1],
					entry->source_ip.ipv6[2],
					entry->source_ip.ipv6[3],
					entry->source_ip.ipv6[4],
					entry->source_ip.ipv6[5],
					entry->source_ip.ipv6[6],
					entry->source_ip.ipv6[7],
					entry->source_ip.ipv6[8],
					entry->source_ip.ipv6[9],
					entry->source_ip.ipv6[10],
					entry->source_ip.ipv6[11],
					entry->source_ip.ipv6[12],
					entry->source_ip.ipv6[13],
					entry->source_ip.ipv6[14],
					entry->source_ip.ipv6[15]);
				fprintf(stdout, "    DestinationIPAddress %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
					entry->destination_ip.ipv6[0],
					entry->destination_ip.ipv6[1],
					entry->destination_ip.ipv6[2],
					entry->destination_ip.ipv6[3],
					entry->destination_ip.ipv6[4],
					entry->destination_ip.ipv6[5],
					entry->destination_ip.ipv6[6],
					entry->destination_ip.ipv6[7],
					entry->destination_ip.ipv6[8],
					entry->destination_ip.ipv6[9],
					entry->destination_ip.ipv6[10],
					entry->destination_ip.ipv6[11],
					entry->destination_ip.ipv6[12],
					entry->destination_ip.ipv6[13],
					entry->destination_ip.ipv6[14],
					entry->destination_ip.ipv6[15]);
				}
			fprintf(stdout, "Port %d\n", entry->port);
			fprintf(stdout, "TSI %d\n", entry->tsi);
			fprintf(stdout, "\n");
		}
	}

	// ESGContainer
	if (strncmp(container_filename, "", MAX_FILENAME) != 0) {
		fprintf(stdout, "**************************************************\n");
		fprintf(stdout, "Reading ESG Container = %s\n", container_filename);
		fprintf(stdout, "**************************************************\n\n");

		read_from_file(container_filename, &buffer, &size);

		struct esg_container *container = esg_container_decode((uint8_t *) buffer, size);
		free(buffer);
		if (container == NULL) {
			fprintf(stderr, "ESG Container decode error\n");
			exit(1);
		}
		if (container->header == NULL) {
			fprintf(stderr, "ESG Container no header found\n");
			exit(1);
		}

		struct esg_encapsulation_structure *fragment_management_information = NULL;
		struct esg_data_repository *data_repository = NULL;
		struct esg_string_repository *string_repository = NULL;
		struct esg_container_structure *structure = NULL;
		struct esg_init_message *init_message = NULL;
		struct esg_textual_encoding_parameters *textual_encoding_parameters = NULL;
		struct esg_textual_decoder_init *textual_decoder_init = NULL;
		struct esg_namespace_prefix *namespace_prefix = NULL;
		struct esg_xml_fragment_type *xml_fragment_type = NULL;
		struct esg_bim_encoding_parameters *bim_encoding_parameters = NULL;
//		struct esg_bim_decoder_init *bim_decoder_init = NULL;
		struct esg_session_partition_declaration *partition = NULL;
		struct esg_session_field *field = NULL;
		struct esg_session_ip_stream *ip_stream = NULL;
		struct esg_session_ip_stream_field *ip_stream_field = NULL;
		esg_container_header_structure_list_for_each(container->header, structure) {
			fprintf(stdout, "    structure_type %d [0x%02x]\n", structure->type, structure->type);
			fprintf(stdout, "    structure_id %d [0x%02x]\n", structure->id, structure->id);
			fprintf(stdout, "    structure_ptr %d\n", structure->ptr);
			fprintf(stdout, "    structure_length %d\n\n", structure->length);
				switch (structure->type) {
				case 0x01: {
					switch (structure->id) {
						case 0x00: {
							fprintf(stdout, "        ESG Fragment Management Information\n");

							fragment_management_information = (struct esg_encapsulation_structure *) structure->data;
							if (fragment_management_information == NULL) {
								fprintf(stderr, "ESG Fragment Management Information decode error\n");
								exit(1);
							}

							fprintf(stdout, "            fragment_reference_format %d [0x%02x]\n\n", fragment_management_information->header->fragment_reference_format, fragment_management_information->header->fragment_reference_format);

							struct esg_encapsulation_entry *entry;
							esg_encapsulation_structure_entry_list_for_each(fragment_management_information, entry) {
								fprintf(stdout, "                fragment_type %d [0x%02x]\n", entry->fragment_reference->fragment_type, entry->fragment_reference->fragment_type);
								fprintf(stdout, "                data_repository_offset %d\n", entry->fragment_reference->data_repository_offset);
								fprintf(stdout, "                fragment_version %d\n", entry->fragment_version);
								fprintf(stdout, "                fragment_id %d\n\n", entry->fragment_id);
							}

							break;
						}
						default: {
							fprintf(stdout, "        Unknown structure_id\n");
						}
					}
					break;
				}
				case 0x02: {
					switch (structure->id) {
						case 0x00: {
							fprintf(stdout, "        ESG String Repository / ");

							string_repository = (struct esg_string_repository *) structure->data;
							if (string_repository == NULL) {
								fprintf(stderr, "ESG String Repository decode error\n");
								exit(1);
							}

							fprintf(stdout, "encoding_type %d / length %d\n\n", string_repository->encoding_type, string_repository->length);

							break;
						}
						default: {
							fprintf(stdout, "        Unknown structure_id\n");
						}
					}
					break;
				}
				case 0x03: {
					//TODO
					break;
				}
				case 0x04: {
					//TODO
					break;
				}
				case 0x05: {
					//TODO
					break;
				}
				case 0xE0: {
					switch (structure->id) {
						case 0x00: {
							fprintf(stdout, "        ESG Data Repository / ");

							data_repository = (struct esg_data_repository *) structure->data;
							if (data_repository == NULL) {
								fprintf(stderr, "ESG Data Repository decode error\n");
								exit(1);
							}

							fprintf(stdout, "length %d\n\n", data_repository->length);

							break;
						}
						default: {
							fprintf(stdout, "        Unknown structure_id\n");
						}
					}
					break;
				}
				case 0xE1: {
					switch (structure->id) {
						case 0xFF: {
							fprintf(stdout, "        ESG Session Partition Declaration\n");

							partition = (struct esg_session_partition_declaration *) structure->data;
							fprintf(stdout, "            num_fields %d\n", partition->num_fields);
							fprintf(stdout, "            overlapping %d\n\n", partition->overlapping);
							esg_session_partition_declaration_field_list_for_each(partition, field) {
								fprintf(stdout, "                identifier %d\n", field->identifier);
								fprintf(stdout, "                encoding %d\n",field->encoding);
								fprintf(stdout, "                length %d\n\n",field->length);
							}
							fprintf(stdout, "            n_o_IPStreams %d\n", partition->n_o_ip_streams);
							fprintf(stdout, "            IPVersion6 %d\n\n", partition->ip_version_6);
							esg_session_partition_declaration_ip_stream_list_for_each(partition, ip_stream) {
								fprintf(stdout, "                IPStreamID %d\n", ip_stream->id);
								if (partition->ip_version_6 == 0) {
									fprintf(stdout, "                SourceIPAddress %d.%d.%d.%d\n",
										ip_stream->source_ip.ipv4[0],
										ip_stream->source_ip.ipv4[1],
										ip_stream->source_ip.ipv4[2],
										ip_stream->source_ip.ipv4[3]);
									fprintf(stdout, "                DestinationIPAddress %d.%d.%d.%d\n",
										ip_stream->destination_ip.ipv4[0],
										ip_stream->destination_ip.ipv4[1],
										ip_stream->destination_ip.ipv4[2],
										ip_stream->destination_ip.ipv4[3]);
								} else if (partition->ip_version_6 == 1) {
									fprintf(stdout, "                SourceIPAddress %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
										ip_stream->source_ip.ipv6[0],
										ip_stream->source_ip.ipv6[1],
										ip_stream->source_ip.ipv6[2],
										ip_stream->source_ip.ipv6[3],
										ip_stream->source_ip.ipv6[4],
										ip_stream->source_ip.ipv6[5],
										ip_stream->source_ip.ipv6[6],
										ip_stream->source_ip.ipv6[7],
										ip_stream->source_ip.ipv6[8],
										ip_stream->source_ip.ipv6[9],
										ip_stream->source_ip.ipv6[10],
										ip_stream->source_ip.ipv6[11],
										ip_stream->source_ip.ipv6[12],
										ip_stream->source_ip.ipv6[13],
										ip_stream->source_ip.ipv6[14],
										ip_stream->source_ip.ipv6[15]);
									fprintf(stdout, "                DestinationIPAddress %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
										ip_stream->destination_ip.ipv6[0],
										ip_stream->destination_ip.ipv6[1],
										ip_stream->destination_ip.ipv6[2],
										ip_stream->destination_ip.ipv6[3],
										ip_stream->destination_ip.ipv6[4],
										ip_stream->destination_ip.ipv6[5],
										ip_stream->destination_ip.ipv6[6],
										ip_stream->destination_ip.ipv6[7],
										ip_stream->destination_ip.ipv6[8],
										ip_stream->destination_ip.ipv6[9],
										ip_stream->destination_ip.ipv6[10],
										ip_stream->destination_ip.ipv6[11],
										ip_stream->destination_ip.ipv6[12],
										ip_stream->destination_ip.ipv6[13],
										ip_stream->destination_ip.ipv6[14],
										ip_stream->destination_ip.ipv6[15]);
								}
								fprintf(stdout, "                Port %d\n", ip_stream->port);
								fprintf(stdout, "                SessionID %d\n", ip_stream->session_id);

								field = partition->field_list;
								esg_session_ip_stream_field_list_for_each(ip_stream, ip_stream_field) {
									switch (field->encoding) {
										case 0x0000: {
											if (ip_stream_field->start_field_value != NULL) {
												fprintf(stdout, "                start_field_value %s\n", ip_stream_field->start_field_value->string);
											}
											fprintf(stdout, "                end_field_value %s\n", ip_stream_field->end_field_value->string);
											break;
										}
										case 0x0101: {
											if (ip_stream_field->start_field_value != NULL) {
												fprintf(stdout, "                start_field_value %d\n", ip_stream_field->start_field_value->unsigned_short);
											}
											fprintf(stdout, "                end_field_value %d\n", ip_stream_field->end_field_value->unsigned_short);
											break;
										}
									}

									field = field->_next;
								}
								fprintf(stdout, "\n");
							}
							break;
						}
						default: {
							fprintf(stdout, "        Unknown structure_id\n");
						}
					}
					break;
				}
				case 0xE2: {
					switch (structure->id) {
						case 0x00: {
							fprintf(stdout, "        ESG Init Message\n");

							init_message = (struct esg_init_message *) structure->data;
							if (init_message == NULL) {
								fprintf(stderr, "ESG Init Message decode error\n");
								exit(1);
							}

							fprintf(stdout, "            EncodingVersion %d [0x%02x]\n", init_message->encoding_version, init_message->encoding_version);
							fprintf(stdout, "            IndexingFlag %d\n", init_message->indexing_flag);
							fprintf(stdout, "            DecoderInitptr %d\n", init_message->decoder_init_ptr);
							if (init_message->indexing_flag) {
								fprintf(stdout, "            IndexingVersion %d\n", init_message->indexing_version);
							}

							switch (init_message->encoding_version) {
								case 0xF1: {
									bim_encoding_parameters = (struct esg_bim_encoding_parameters *) init_message->encoding_parameters;
									if (bim_encoding_parameters == NULL) {
										fprintf(stderr, "ESG Init Message decode error / bim_encoding_parameters\n");
										exit(1);
									}
									fprintf(stdout, "            BufferSizeFlag %d\n", bim_encoding_parameters->buffer_size_flag);
									fprintf(stdout, "            PositionCodeFlag %d\n", bim_encoding_parameters->position_code_flag);
									fprintf(stdout, "            CharacterEncoding %d\n", bim_encoding_parameters->character_encoding);
									if (bim_encoding_parameters->buffer_size_flag) {
										fprintf(stdout, "            BufferSize %d\n", bim_encoding_parameters->buffer_size);
									}

									// TODO BimDecoderInit
									break;
								}
								case 0xF2:
								case 0xF3: {
									textual_encoding_parameters = (struct esg_textual_encoding_parameters *) init_message->encoding_parameters;
									if (textual_encoding_parameters == NULL) {
										fprintf(stderr, "ESG Init Message decode error / textual_encoding_parameters\n");
										exit(1);
									}
									fprintf(stdout, "            CharacterEncoding %d\n\n", textual_encoding_parameters->character_encoding);

									// TextualDecoderInit
									textual_decoder_init = (struct esg_textual_decoder_init *) init_message->decoder_init;
									if (textual_decoder_init == NULL) {
										fprintf(stderr, "ESG Init Message decode error / textual_decoder_init\n");
										exit(1);
									}
									fprintf(stdout, "            Textual DecoderInit\n");
									fprintf(stdout, "                num_namespaces_prefixes %d\n\n", textual_decoder_init->num_namespace_prefixes);
									esg_textual_decoder_namespace_prefix_list_for_each(textual_decoder_init, namespace_prefix) {
										fprintf(stdout, "                    prefix_string_ptr %d\n", namespace_prefix->prefix_string_ptr);
										fprintf(stdout, "                    namespace_URI_ptr %d\n\n", namespace_prefix->namespace_uri_ptr);
									}
									fprintf(stdout, "                num_fragment_types %d\n\n", textual_decoder_init->num_fragment_types);
									esg_textual_decoder_xml_fragment_type_list_for_each(textual_decoder_init, xml_fragment_type) {
										fprintf(stdout, "                    xpath_ptr %d\n", xml_fragment_type->xpath_ptr);
										fprintf(stdout, "                    ESG_XML_fragment_type %d\n\n", xml_fragment_type->xml_fragment_type);
									}
									break;
								}
								default: {
									fprintf(stdout, "            Unknown EncodingVersion\n");
								}
							}

							break;
						}
						default: {
							fprintf(stdout, "        Unknown structure_id\n");
						}
					}
					break;
				}
				default: {
					fprintf(stdout, "        Unknown structure_type\n");
				}
			}
		}
		fprintf(stdout, "\n");

		fprintf(stdout, "structure_body_ptr %d\n", container->structure_body_ptr);
		fprintf(stdout, "structure_body_length %d\n\n", container->structure_body_length);

		// ESG XML Fragment
		if (fragment_management_information) {
			fprintf(stdout, "**************************************************\n");
			fprintf(stdout, "ESG XML Fragment\n");
			fprintf(stdout, "**************************************************\n\n");

			struct esg_encapsulation_entry *entry;
			esg_encapsulation_structure_entry_list_for_each(fragment_management_information, entry) {
				switch (entry->fragment_reference->fragment_type) {
					case 0x00: {
						if (data_repository) {
							struct esg_encapsulated_textual_esg_xml_fragment *esg_xml_fragment = esg_encapsulated_textual_esg_xml_fragment_decode(data_repository->data + entry->fragment_reference->data_repository_offset, data_repository->length);

							fprintf(stdout, "ESG_XML_fragment_type %d\n", esg_xml_fragment->esg_xml_fragment_type);
							fprintf(stdout, "data_length %d\n", esg_xml_fragment->data_length);
							fprintf(stdout, "fragment_version %d\n", entry->fragment_version);
							fprintf(stdout, "fragment_id %d\n\n", entry->fragment_id);
							char *string = (char *) malloc(esg_xml_fragment->data_length + 1);
							memcpy(string, esg_xml_fragment->data, esg_xml_fragment->data_length);
							string[esg_xml_fragment->data_length] = 0;
							fprintf(stdout, "%s\n", string);

						} else {
							fprintf(stderr, "ESG Data Repository not found");
						}
						break;
					}
					case 0x01: {
						// TODO
						break;
					}
					case 0x02: {
						// TODO
						break;
					}
					default: {
					}
				}
			}
		}

		// String
		if (init_message) {
			fprintf(stdout, "**************************************************\n");
			fprintf(stdout, "String\n");
			fprintf(stdout, "**************************************************\n\n");

			switch (init_message->encoding_version) {
				case 0xF1: {
					// TODO Bim
					break;
				}
				case 0xF2: {
					// TODO GZIP
					break;
				}
				case 0xF3: {
					// RAW
					if (string_repository) {
						textual_decoder_init = (struct esg_textual_decoder_init *) init_message->decoder_init;
						esg_textual_decoder_namespace_prefix_list_for_each(textual_decoder_init, namespace_prefix) {
							fprintf(stdout, "prefix_string_ptr %d\n", namespace_prefix->prefix_string_ptr);
							fprintf(stdout, "%s\n", string_repository->data + namespace_prefix->prefix_string_ptr);
							fprintf(stdout, "namespace_URI_ptr %d\n", namespace_prefix->namespace_uri_ptr);
							fprintf(stdout, "%s\n\n", string_repository->data + namespace_prefix->namespace_uri_ptr - 1); // TODO -1
						}

						esg_textual_decoder_xml_fragment_type_list_for_each(textual_decoder_init, xml_fragment_type) {
							fprintf(stdout, "xpath_ptr %d\n", xml_fragment_type->xpath_ptr);
							fprintf(stdout, "ESG_XML_fragment_type %d\n", xml_fragment_type->xml_fragment_type);
							fprintf(stdout, "%s\n\n", string_repository->data + xml_fragment_type->xpath_ptr - 1); // TODO -1
						}
					}
					break;
				}
				default: {
					fprintf(stdout, "            Unknown EncodingVersion\n");
				}
			}
		}
	}

	return 0;
}
