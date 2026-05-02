const express = require('express');
const { createLogger, format, transports } = require('winston');

// [Winston 로거 설정 — 날짜별 파일 + 콘솔 동시 출력]
const logger = createLogger({
    level: 'info',
    format: format.combine(
        format.timestamp(),
        format.printf(info => `${info.timestamp} ${info.level}: ${info.message}`)
    ),
    transports: [
        new transports.File({
            filename: `logs/node_${new Date().toISOString().split('T')[0]}.log`
        }),
        new transports.Console()
    ]
});

const app = express();
const PORT = 3000;

app.use(express.json());

// [라우터 등록]
const characterRouter = require('./routes/character');
const auctionRouter   = require('./routes/auction');
const adminRouter     = require('./routes/admin');
const rankingRouter   = require('./routes/ranking');

app.use(characterRouter);
app.use(auctionRouter);
app.use(adminRouter);
app.use(rankingRouter);

// [전역 에러 핸들러]
app.use((err, req, res, next) => {
    logger.error('전역 에러 발생: ' + err.stack);
    res.status(500).json({ message: '서버 오류가 발생했습니다.' });
});

app.listen(PORT, () => {
    logger.info('서버 시작. Port: ' + PORT);
});

module.exports = { logger };
