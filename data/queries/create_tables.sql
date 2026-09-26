CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,

    hash BLOB NOT NULL,
    salt BLOB NOT NULL
);

CREATE TABLE IF NOT EXISTS videos (
    id TEXT PRIMARY KEY NOT NULL UNIQUE,
    channel INTEGER NOT NULL,

    title TEXT,
    description TEXT,
    views INTEGER NOT NULL,
    creation_time INTEGER NOT NULL,
    resolutions TEXT,

    state INTEGER NOT NULL,
    size INTEGER
);
