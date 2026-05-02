const Redis  = require('ioredis');
const { logger } = require('./server');

// [Redis 연결]
const redis = new Redis({ host: '127.0.0.1', port: 6379 });

redis.on('connect', () => {
    logger.info('Redis 연결 완료');
});

redis.on('error', (err) => {
    logger.error('Redis 오류: ' + err.message);
});

module.exports = redis;
