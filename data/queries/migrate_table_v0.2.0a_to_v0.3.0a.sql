CREATE TABLE comments (
    id INTEGER PRIMARY KEY NOT NULL AUTOINCREMENT,

    channel INTEGER NOT NULL,
    video TEXT NOT NULL,
    creation_time INTEGER NOT NULL,

    content TEXT NOT NULL
);
