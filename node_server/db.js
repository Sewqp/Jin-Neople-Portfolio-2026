const mysql = require('mysql2/promise');

// [커넥션 풀 크기 상수]
const POOL_SIZE = 10;

// [MySQL 커넥션 풀 생성]
const pool = mysql.createPool({
    host:            '127.0.0.1',
    user:            'root',
    password:        'password',
    database:        'game_server_schema',
    connectionLimit: POOL_SIZE
});

module.exports = pool;
