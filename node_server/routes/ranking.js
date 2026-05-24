const express = require('express');
const router  = express.Router();
const pool    = require('../db');
const logger  = require('../logger');

const MAX_RANKINGS = 100;

async function getTopCharactersByLevel() {
    const [rows] = await pool.execute(`
        SELECT c.character_id, c.nickname, c.job_code, cs.level
        FROM \`character\` AS c
        JOIN character_stat AS cs ON c.character_id = cs.character_id
        ORDER BY cs.level DESC, c.created_at ASC
        LIMIT ${MAX_RANKINGS}
    `);
    return rows.map((row, index) => ({ rank: index + 1, ...row }));
}

async function getTopAuctionsByPrice() {
    const [rows] = await pool.execute(`
        SELECT id.item_name, a.price, ii.enhance_level, a.seller_id
        FROM auction AS a
        JOIN item_instance AS ii ON a.item_instance_id = ii.item_instance_id
        JOIN item_dictionary AS id ON ii.item_dict_id = id.item_dict_id
        WHERE a.trade_status = 1
        ORDER BY a.price DESC, a.expired_at ASC
        LIMIT ${MAX_RANKINGS}
    `);
    return rows.map((row, index) => ({ rank: index + 1, ...row }));
}

router.get('/api/ranking/level', async (req, res) => {
    try {
        const topCharacters = await getTopCharactersByLevel();
        res.json(topCharacters);
    } catch (error) {
        logger.error('레벨 랭킹 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

router.get('/api/ranking/auction', async (req, res) => {
    try {
        const topAuctions = await getTopAuctionsByPrice();
        res.json(topAuctions);
    } catch (error) {
        logger.error('경매 랭킹 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

module.exports = router;