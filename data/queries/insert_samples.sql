/* Insert user "username" with password "password" */
INSERT INTO users (name, hash, salt) VALUES (
    'username',
    x'a68d6f466720bf4f5e7dcf23483fe38a7c0d435f185c323c4619bac05d58809d',
    x'5b8672761737832fb9ca0d80abe94efa'
);

/* Insert a couple videos from the user */
INSERT INTO videos (id, channel, title, views, creation_time) VALUES (
    'abc123',
    (SELECT id FROM users WHERE name = 'username'),
    'Video Title',
    7600000,
    unixepoch('2024-01-12 06:25:12')
);

INSERT INTO videos (id, channel, title, views, creation_time) VALUES (
    'abcdef',
    (SELECT id FROM users WHERE name = 'username'),
    'Second Video',
    928,
    unixepoch('2026-09-24 03:00:00')
);
