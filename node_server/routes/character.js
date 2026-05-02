const express = require('express');
const router  = express.Router();
const pool    = require('../db');
const redis   = require('../redis');
const { logger } = require('../server');
const { CHARACTER_STAT_CACHE_KEY } = require('../constants');

// [캐릭터 기본 정보 + 스탯 조회]
router.get('/api/character/:characterId', async (req, res) => {
    const characterId = req.params.characterId;
    const cacheKey    = `${CHARACTER_STAT_CACHE_KEY}${characterId}`;

    try {
        // [Redis 캐시 먼저 확인]
        const cachedStat = await redis.get(cacheKey);
        if (cachedStat) {
            return res.json(JSON.parse(cachedStat));
        }

        // [캐시 미스 → MySQL 조회]
        const [rows] = await pool.execute(`
            SELECT c.character_id, c.nickname, c.job_code,
                   s.level, s.hp, s.mp
            FROM character AS c
            JOIN character_stat AS s ON c.character_id = s.character_id
            WHERE c.character_id = ?
        `, [characterId]);

        if (rows.length === 0) {
            return res.status(404).json({ message: '캐릭터를 찾을 수 없습니다.' });
        }

        const stat = rows[0];

        // [Redis에 캐시 저장 — 1시간 만료]
        await redis.set(cacheKey, JSON.stringify(stat), 'EX', 3600);

        res.json(stat);
    } catch (error) {
        logger.error('캐릭터 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

// [캐릭터 인벤토리 조회]
router.get('/api/character/:characterId/inventory', async (req, res) => {
    const characterId = req.params.characterId;
    const tabType     = req.query.tab_type || 0;

    try {
        const [rows] = await pool.execute(`
            SELECT i.slot_index, id.item_name, id.item_type,
                   ii.count, ii.enhance_level
            FROM inventory AS i
            JOIN item_instance AS ii ON i.item_instance_id = ii.item_instance_id
            JOIN item_dictionary AS id ON ii.item_dict_id = id.item_dict_id
            WHERE i.character_id = ? AND i.tab_type = ?
        `, [characterId, tabType]);

        res.json(rows);
    } catch (error) {
        logger.error('인벤토리 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

module.exports = router;
