const { createLogger, format, transports, Transport } = require('winston');
const fs       = require('fs');
const Anthropic = require('@anthropic-ai/sdk');

fs.mkdirSync('logs', { recursive: true });

const anthropic = new Anthropic({ apiKey: process.env.ANTHROPIC_API_KEY });

async function analyzeError(message) {
    try {
        const response = await anthropic.messages.create({
            model: 'claude-haiku-4-5-20251001',
            max_tokens: 512,
            messages: [{ role: 'user', content: `서버 에러 로그입니다. 원인과 조치 방법을 간략히 분석해주세요.\n\n${message}` }]
        });
        return response.content[0].text;
    } catch {
        return null;
    }
}

// 에러 폭발 시 API 과호출 방지
const recentErrors      = new Map();
const ERROR_COOLDOWN_MS = 60_000;

// winston File 트랜스포트는 파일명이 시작 시점에 고정되므로
// 날짜 롤오버 대응을 위해 직접 구현
class DailyFileTransport extends Transport {
    log(info, callback) {
        const date = new Date().toISOString().split('T')[0].replace(/-/g, '');
        fs.appendFile(`logs/node_${date}.log`, info[Symbol.for('message')] + '\n', () => callback());
    }
}

class LLMErrorTransport extends Transport {
    log(info, callback) {
        if (info.level === 'error') {
            const now  = Date.now();
            const last = recentErrors.get(info.message) ?? 0;
            if (now - last > ERROR_COOLDOWN_MS) {
                recentErrors.set(info.message, now);
                analyzeError(info.message).then(analysis => {
                    if (analysis) logger.info('[LLM 분석] ' + analysis);
                });
            }
        }
        callback();
    }
}

const logger = createLogger({
    level: 'info',
    format: format.combine(
        format.timestamp(),
        format.printf(info => `${info.timestamp} ${info.level}: ${info.message}`)
    ),
    transports: [
        new DailyFileTransport(),
        new transports.Console(),
        new LLMErrorTransport()
    ]
});

module.exports = logger;
