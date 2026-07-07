// study_planner.js
const fs = require('fs').promises;
const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const question = (prompt) => new Promise(resolve => rl.question(prompt, resolve));

class StudySession {
    constructor(id, subject, topic, datetime, duration, priority, status = 'Запланирована') {
        this.id = id;
        this.subject = subject;
        this.topic = topic;
        this.datetime = datetime;
        this.duration = duration;
        this.priority = priority;
        this.status = status;
    }
}

class StudyPlanner {
    constructor() {
        this.sessions = [];
        this.nextId = 1;
    }

    validateDatetime(dt) {
        const pattern = /^\d{4}-\d{2}-\d{2} \d{2}:\d{2}$/;
        if (!pattern.test(dt)) throw new Error('Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ');
        const date = new Date(dt);
        if (isNaN(date)) throw new Error('Неверная дата');
    }

    addSession(subject, topic, datetime, duration, priority) {
        this.validateDatetime(datetime);
        if (duration <= 0) throw new Error('Длительность должна быть положительной');
        const session = new StudySession(this.nextId, subject, topic, datetime, duration, priority);
        this.sessions.push(session);
        this.nextId++;
        return session;
    }

    findSession(id) {
        return this.sessions.find(s => s.id === id);
    }

    updateSession(id, updates) {
        const session = this.findSession(id);
        if (!session) return false;
        Object.assign(session, updates);
        return true;
    }

    deleteSession(id) {
        const index = this.sessions.findIndex(s => s.id === id);
        if (index === -1) return false;
        this.sessions.splice(index, 1);
        return true;
    }

    setStatus(id, status) {
        const session = this.findSession(id);
        if (!session) return false;
        session.status = status;
        return true;
    }

    filterBySubject(subject) {
        return this.sessions.filter(s => s.subject.toLowerCase() === subject.toLowerCase());
    }

    filterByStatus(status) {
        return this.sessions.filter(s => s.status === status);
    }

    filterByDate(dateStr) {
        return this.sessions.filter(s => s.datetime.startsWith(dateStr));
    }

    getStats() {
        const total = this.sessions.length;
        const completed = this.filterByStatus('Выполнена').length;
        const planned = this.filterByStatus('Запланирована').length;
        const missed = this.filterByStatus('Пропущена').length;
        const totalTime = this.sessions.reduce((sum, s) => sum + s.duration, 0);
        const completedTime = this.filterByStatus('Выполнена').reduce((sum, s) => sum + s.duration, 0);
        const avgDuration = total > 0 ? totalTime / total : 0;
        const bySubject = {};
        this.sessions.forEach(s => {
            bySubject[s.subject] = (bySubject[s.subject] || 0) + s.duration;
        });
        return { total, completed, planned, missed, totalTime, completedTime, avgDuration, bySubject };
    }

    async saveToFile(filename = 'sessions_data.json') {
        const data = { sessions: this.sessions };
        await fs.writeFile(filename, JSON.stringify(data, null, 2));
    }

    async loadFromFile(filename = 'sessions_data.json') {
        try {
            const data = await fs.readFile(filename, 'utf8');
            const parsed = JSON.parse(data);
            this.sessions = parsed.sessions.map(s => Object.assign(new StudySession(0), s));
            this.nextId = this.sessions.reduce((max, s) => Math.max(max, s.id), 0) + 1;
        } catch (err) {
            if (err.code !== 'ENOENT') throw err;
        }
    }
}

function printSession(session) {
    const emoji = { 'Запланирована': '📅', 'Выполнена': '✅', 'Пропущена': '❌' };
    console.log(`${emoji[session.status] || '📅'} #${session.id} - ${session.subject} (${session.topic})`);
    console.log(`   Дата/время: ${session.datetime}, Длительность: ${session.duration} мин`);
    console.log(`   Приоритет: ${session.priority}, Статус: ${session.status}`);
}

async function main() {
    const planner = new StudyPlanner();
    await planner.loadFromFile();

    while (true) {
        console.log('\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (JavaScript) =====');
        console.log('1. Добавить сессию');
        console.log('2. Показать все сессии');
        console.log('3. Показать сессии по предмету');
        console.log('4. Отметить сессию как выполненную');
        console.log('5. Редактировать сессию');
        console.log('6. Удалить сессию');
        console.log('7. Показать статистику');
        console.log('8. Сохранить в файл');
        console.log('9. Загрузить из файла');
        console.log('0. Выход');
        const choice = await question('Выберите действие: ');

        if (choice === '0') break;

        switch (choice) {
            case '1': {
                const subject = await question('Предмет: ');
                if (!subject.trim()) { console.log('Предмет не может быть пустым.'); continue; }
                const topic = await question('Тема: ');
                const datetime = await question('Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ');
                const duration = parseInt(await question('Длительность (мин): '));
                console.log('Приоритет: 1-Низкий, 2-Средний, 3-Высокий');
                const prioInput = await question('> ');
                let priority;
                if (prioInput === '1') priority = 'Низкий';
                else if (prioInput === '3') priority = 'Высокий';
                else priority = 'Средний';
                try {
                    const session = planner.addSession(subject, topic, datetime, duration, priority);
                    console.log(`Сессия добавлена с ID ${session.id}`);
                } catch (err) {
                    console.log('Ошибка:', err.message);
                }
                break;
            }
            case '2':
                if (planner.sessions.length === 0) console.log('Нет сессий.');
                else planner.sessions.forEach(printSession);
                break;
            case '3': {
                const subj = await question('Введите предмет: ');
                const filtered = planner.filterBySubject(subj);
                if (filtered.length === 0) console.log('Сессий по этому предмету нет.');
                else filtered.forEach(printSession);
                break;
            }
            case '4': {
                const id = parseInt(await question('Введите ID сессии: '));
                if (planner.setStatus(id, 'Выполнена')) {
                    console.log('Сессия отмечена как выполненная.');
                } else {
                    console.log('Сессия не найдена.');
                }
                break;
            }
            case '5': {
                const id = parseInt(await question('Введите ID сессии для редактирования: '));
                const session = planner.findSession(id);
                if (!session) { console.log('Сессия не найдена.'); continue; }
                console.log('Оставьте поле пустым, чтобы не менять.');
                const newSubj = await question(`Предмет (${session.subject}): `);
                const newTopic = await question(`Тема (${session.topic}): `);
                const newDt = await question(`Дата/время (${session.datetime}): `);
                const newDur = await question(`Длительность (${session.duration}): `);
                const newPrio = await question(`Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: ${session.priority}: `);
                const updates = {};
                if (newSubj.trim()) updates.subject = newSubj;
                if (newTopic.trim()) updates.topic = newTopic;
                if (newDt.trim()) updates.datetime = newDt;
                if (newDur.trim()) {
                    const dur = parseInt(newDur);
                    if (!isNaN(dur)) updates.duration = dur;
                    else console.log('Длительность должна быть числом, пропускаем.');
                }
                if (newPrio.trim()) {
                    if (newPrio === '1') updates.priority = 'Низкий';
                    else if (newPrio === '3') updates.priority = 'Высокий';
                    else updates.priority = 'Средний';
                }
                if (planner.updateSession(id, updates)) {
                    console.log('Сессия обновлена.');
                } else {
                    console.log('Ошибка обновления.');
                }
                break;
            }
            case '6': {
                const id = parseInt(await question('Введите ID сессии для удаления: '));
                if (planner.deleteSession(id)) {
                    console.log('Сессия удалена.');
                } else {
                    console.log('Сессия не найдена.');
                }
                break;
            }
            case '7': {
                const stats = planner.getStats();
                console.log('\n=== СТАТИСТИКА ===');
                console.log(`Всего сессий: ${stats.total}`);
                console.log(`Выполнено: ${stats.completed}`);
                console.log(`Запланировано: ${stats.planned}`);
                console.log(`Пропущено: ${stats.missed}`);
                console.log(`Общее время: ${stats.totalTime} мин`);
                console.log(`Время выполненных: ${stats.completedTime} мин`);
                console.log(`Средняя длительность: ${stats.avgDuration.toFixed(1)} мин`);
                console.log('По предметам:');
                for (const [subj, mins] of Object.entries(stats.bySubject)) {
                    console.log(`  ${subj}: ${mins} мин`);
                }
                break;
            }
            case '8':
                try {
                    await planner.saveToFile();
                    console.log('Сохранено.');
                } catch (err) {
                    console.log('Ошибка сохранения:', err.message);
                }
                break;
            case '9':
                try {
                    await planner.loadFromFile();
                    console.log('Загружено.');
                } catch (err) {
                    console.log('Ошибка загрузки:', err.message);
                }
                break;
            default:
                console.log('Неизвестная команда.');
        }
    }
    rl.close();
}

main().catch(console.error);
