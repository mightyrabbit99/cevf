/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#ifndef IDS_H
#define IDS_H

struct ids_s;
typedef int ids_id_t;

struct ids_s *ids_new_idsys(void);
void ids_free_idsys(struct ids_s *idsys);
ids_id_t ids_add_item(struct ids_s *idsys, void *item);
void *ids_get_item(struct ids_s *idsys, ids_id_t id);
int ids_rm_item(struct ids_s *idsys, ids_id_t id);
void ids_foreach(struct ids_s *idsys, typeof(void(void *)) f);
#endif // IDS_H
