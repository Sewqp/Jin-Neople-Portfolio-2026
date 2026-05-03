const express = require('express');
const router  = express.Router();
const fs      = require('fs');
const pool    = require('../db');
const redis   = require('../redis');
const { logger } = require('../server');

// [최대 로그 줄 수 상수]
const MAX_LOG_LINES = 100;

// [온라인 플레이어 수 — Redis session:* 키 개수]
async function getOnlinePlayers() {
    const keys = await redis.keys('session:*');
    return keys.length;
}

// [전체 캐릭터 수 — MySQL]
async function getTotalCharacters() {
    const [rows] = await pool.execute('SELECT COUNT(*) AS count FROM character');
    return rows[0].count;
}

// [진행 중인 경매 수 — MySQL]
async function getActiveAuctions() {
    const [rows] = await pool.execute(
        'SELECT COUNT(*) AS count FROM auction WHERE trade_status = 0');
    return rows[0].count;
}

// [서버 상태 조회]
router.get('/api/admin/status', async (req, res) => {
    try {
        const onlinePlayers   = await getOnlinePlayers();
        const totalCharacters = await getTotalCharacters();
        const activeAuctions  = await getActiveAuctions();
        res.json({ onlinePlayers, totalCharacters, activeAuctions });
    } catch (error) {
        logger.error('관리자 상태 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

// [로그 파일 마지막 N줄 읽기]
async function readLogs(logName, lines) {
    const filePath = `logs/${logName}`;
    try {
        const content = await fs.promises.readFile(filePath, 'utf-8');
        return content.split('\n').slice(-lines);
    } catch {
        return [];
    }
}

// [로그 조회 — C++ 서버 로그 + Node 로그]
router.get('/api/admin/logs', async (req, res) => {
    const lines = Number(req.query.lines) || MAX_LOG_LINES;
    try {
        const date       = new Date().toISOString().split('T')[0].replace(/-/g, '');
        const serverLogs = await readLogs(`server_${date}.log`, lines);
        const nodeLogs   = await readLogs(`node_${date}.log`,   lines);
        res.json({ serverLogs, nodeLogs });
    } catch (error) {
        logger.error('로그 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

module.exports = router;
