-- Don't modify this! Create a new migration instead--see docs/schema-migrations.adoc
BEGIN TRANSACTION;

DROP TABLE root_meta;
DROP TABLE rawmeta;
DROP TABLE root_rawmeta;

CREATE TABLE repo_types(repo INTEGER NOT NULL, repo_string TEXT NOT NULL);
INSERT INTO repo_types VALUES (0, 'director'), (1, 'images');

CREATE TABLE meta_types(meta INTEGER NOT NULL, meta_string TEXT NOT NULL);
INSERT INTO meta_types VALUES (0, 'root'), (1, 'snapshot'), (2, 'targets'), (3, 'timestamp');

CREATE TABLE meta_migrate(meta BLOB NOT NULL, repo INTEGER NOT NULL, meta_type INTEGER NOT NULL, version INTEGER NOT NULL);
INSERT INTO meta_migrate SELECT director_root, 0, 0, -1 FROM meta LIMIT 1;
INSERT INTO meta_migrate SELECT director_targets, 0, 1, -1 FROM meta LIMIT 1;
INSERT INTO meta_migrate SELECT image_root, 1, 0, -1 FROM meta LIMIT 1;
INSERT INTO meta_migrate SELECT image_targets, 1, 1, -1 FROM meta LIMIT 1;
INSERT INTO meta_migrate SELECT image_timestamp, 1, 2, -1 FROM meta LIMIT 1;
INSERT INTO meta_migrate SELECT image_snapshot, 1, 3, -1 FROM meta LIMIT 1;

DROP TABLE meta;
ALTER TABLE meta_migrate RENAME TO meta;

DELETE FROM version;
INSERT INTO version VALUES(6);

COMMIT TRANSACTION;
