CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    hash TEXT NOT NULL,
    salt TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS videos (
    uuid TEXT NOT NULL UNIQUE,
    channel INTEGER NOT NULL,

    title TEXT,
    description TEXT,
    views INTEGER NOT NULL,
    creation_time INTEGER NOT NULL
);
