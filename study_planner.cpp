// study_planner.cpp
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <map>
#include <variant>
#include <regex>
#include <cctype>

using namespace std;

enum class Priority { LOW, MEDIUM, HIGH };
enum class SessionStatus { PLANNED, COMPLETED, MISSED };

string priorityToString(Priority p) {
    switch (p) {
        case Priority::LOW: return "Низкий";
        case Priority::MEDIUM: return "Средний";
        case Priority::HIGH: return "Высокий";
        default: return "";
    }
}

Priority stringToPriority(const string& s) {
    if (s == "Низкий") return Priority::LOW;
    if (s == "Высокий") return Priority::HIGH;
    return Priority::MEDIUM;
}

string statusToString(SessionStatus s) {
    switch (s) {
        case SessionStatus::PLANNED: return "Запланирована";
        case SessionStatus::COMPLETED: return "Выполнена";
        case SessionStatus::MISSED: return "Пропущена";
        default: return "";
    }
}

SessionStatus stringToStatus(const string& s) {
    if (s == "Выполнена") return SessionStatus::COMPLETED;
    if (s == "Пропущена") return SessionStatus::MISSED;
    return SessionStatus::PLANNED;
}

struct StudySession {
    int id;
    string subject;
    string topic;
    string datetime;
    int duration;
    Priority priority;
    SessionStatus status;

    StudySession(int id, const string& subject, const string& topic, const string& datetime,
                 int duration, Priority priority, SessionStatus status = SessionStatus::PLANNED)
        : id(id), subject(subject), topic(topic), datetime(datetime),
          duration(duration), priority(priority), status(status) {}
};

class StudyPlanner {
private:
    vector<StudySession> sessions;
    int nextId = 1;

    void validateDatetime(const string& dt) {
        regex pattern(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2})");
        if (!regex_match(dt, pattern)) {
            throw invalid_argument("Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ");
        }
        // Дополнительно проверим, что дата валидна (упрощённо)
    }

public:
    StudySession addSession(const string& subject, const string& topic, const string& datetime,
                            int duration, Priority priority) {
        validateDatetime(datetime);
        if (duration <= 0) throw invalid_argument("Длительность должна быть положительной");
        StudySession session(nextId, subject, topic, datetime, duration, priority);
        sessions.push_back(session);
        nextId++;
        return session;
    }

    StudySession* findSession(int id) {
        auto it = find_if(sessions.begin(), sessions.end(), [id](const StudySession& s) { return s.id == id; });
        return it != sessions.end() ? &(*it) : nullptr;
    }

    bool updateSession(int id, const map<string, string>& updates) {
        StudySession* session = findSession(id);
        if (!session) return false;
        for (const auto& [key, value] : updates) {
            if (key == "subject") session->subject = value;
            else if (key == "topic") session->topic = value;
            else if (key == "datetime") { validateDatetime(value); session->datetime = value; }
            else if (key == "duration") { session->duration = stoi(value); }
            else if (key == "priority") { session->priority = stringToPriority(value); }
            else if (key == "status") { session->status = stringToStatus(value); }
        }
        return true;
    }

    bool deleteSession(int id) {
        auto it = find_if(sessions.begin(), sessions.end(), [id](const StudySession& s) { return s.id == id; });
        if (it == sessions.end()) return false;
        sessions.erase(it);
        return true;
    }

    bool setStatus(int id, SessionStatus status) {
        StudySession* session = findSession(id);
        if (!session) return false;
        session->status = status;
        return true;
    }

    vector<StudySession> filterBySubject(const string& subject) const {
        vector<StudySession> result;
        for (const auto& s : sessions) {
            if (s.subject == subject) result.push_back(s);
        }
        return result;
    }

    vector<StudySession> filterByStatus(SessionStatus status) const {
        vector<StudySession> result;
        for (const auto& s : sessions) {
            if (s.status == status) result.push_back(s);
        }
        return result;
    }

    vector<StudySession> filterByDate(const string& dateStr) const {
        vector<StudySession> result;
        for (const auto& s : sessions) {
            if (s.datetime.compare(0, dateStr.size(), dateStr) == 0) result.push_back(s);
        }
        return result;
    }

    map<string, variant<int, double, map<string, int>>> getStats() const {
        int total = sessions.size();
        int completed = filterByStatus(SessionStatus::COMPLETED).size();
        int planned = filterByStatus(SessionStatus::PLANNED).size();
        int missed = filterByStatus(SessionStatus::MISSED).size();
        int totalTime = 0;
        int completedTime = 0;
        map<string, int> bySubject;
        for (const auto& s : sessions) {
            totalTime += s.duration;
            if (s.status == SessionStatus::COMPLETED) completedTime += s.duration;
            bySubject[s.subject] += s.duration;
        }
        double avgDuration = total > 0 ? static_cast<double>(totalTime) / total : 0.0;
        map<string, variant<int, double, map<string, int>>> stats;
        stats["total"] = total;
        stats["completed"] = completed;
        stats["planned"] = planned;
        stats["missed"] = missed;
        stats["total_time"] = totalTime;
        stats["completed_time"] = completedTime;
        stats["avg_duration"] = avgDuration;
        stats["by_subject"] = bySubject;
        return stats;
    }

    void saveToFile(const string& filename = "sessions_data.txt") {
        ofstream out(filename);
        if (!out) return;
        for (const auto& s : sessions) {
            out << s.id << '|'
                << s.subject << '|'
                << s.topic << '|'
                << s.datetime << '|'
                << s.duration << '|'
                << priorityToString(s.priority) << '|'
                << statusToString(s.status) << '\n';
        }
    }

    void loadFromFile(const string& filename = "sessions_data.txt") {
        ifstream in(filename);
        if (!in) return;
        sessions.clear();
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string idStr, subject, topic, datetime, durStr, priorityStr, statusStr;
            getline(ss, idStr, '|');
            getline(ss, subject, '|');
            getline(ss, topic, '|');
            getline(ss, datetime, '|');
            getline(ss, durStr, '|');
            getline(ss, priorityStr, '|');
            getline(ss, statusStr, '|');
            int id = stoi(idStr);
            int duration = stoi(durStr);
            Priority priority = stringToPriority(priorityStr);
            SessionStatus status = stringToStatus(statusStr);
            sessions.emplace_back(id, subject, topic, datetime, duration, priority, status);
            if (id >= nextId) nextId = id + 1;
        }
    }

    const vector<StudySession>& getSessions() const { return sessions; }
};

string readString(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        try {
            return stoi(input);
        } catch (...) {
            cout << "Введите число.\n";
        }
    }
}

void printSession(const StudySession& session) {
    string emoji;
    if (session.status == SessionStatus::PLANNED) emoji = "📅";
    else if (session.status == SessionStatus::COMPLETED) emoji = "✅";
    else emoji = "❌";
    cout << emoji << " #" << session.id << " - " << session.subject << " (" << session.topic << ")\n";
    cout << "   Дата/время: " << session.datetime << ", Длительность: " << session.duration << " мин\n";
    cout << "   Приоритет: " << priorityToString(session.priority) << ", Статус: " << statusToString(session.status) << "\n";
}

int main() {
    StudyPlanner planner;
    planner.loadFromFile();

    while (true) {
        cout << "\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (C++) =====" << endl;
        cout << "1. Добавить сессию\n";
        cout << "2. Показать все сессии\n";
        cout << "3. Показать сессии по предмету\n";
        cout << "4. Отметить сессию как выполненную\n";
        cout << "5. Редактировать сессию\n";
        cout << "6. Удалить сессию\n";
        cout << "7. Показать статистику\n";
        cout << "8. Сохранить в файл\n";
        cout << "9. Загрузить из файла\n";
        cout << "0. Выход\n";
        string choice = readString("Выберите действие: ");

        if (choice == "0") break;

        if (choice == "1") {
            string subject = readString("Предмет: ");
            if (subject.empty()) { cout << "Предмет не может быть пустым.\n"; continue; }
            string topic = readString("Тема: ");
            string datetime = readString("Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ");
            int duration = readInt("Длительность (мин): ");
            cout << "Приоритет: 1-Низкий, 2-Средний, 3-Высокий\n";
            string prioInput = readString("> ");
            Priority priority;
            if (prioInput == "1") priority = Priority::LOW;
            else if (prioInput == "3") priority = Priority::HIGH;
            else priority = Priority::MEDIUM;
            try {
                StudySession session = planner.addSession(subject, topic, datetime, duration, priority);
                cout << "Сессия добавлена с ID " << session.id << "\n";
            } catch (const exception& e) {
                cout << "Ошибка: " << e.what() << "\n";
            }
        } else if (choice == "2") {
            if (planner.getSessions().empty()) {
                cout << "Нет сессий.\n";
            } else {
                for (const auto& s : planner.getSessions()) {
                    printSession(s);
                }
            }
        } else if (choice == "3") {
            string subj = readString("Введите предмет: ");
            auto sessions = planner.filterBySubject(subj);
            if (sessions.empty()) {
                cout << "Сессий по этому предмету нет.\n";
            } else {
                for (const auto& s : sessions) printSession(s);
            }
        } else if (choice == "4") {
            int id = readInt("Введите ID сессии: ");
            if (planner.setStatus(id, SessionStatus::COMPLETED)) {
                cout << "Сессия отмечена как выполненная.\n";
            } else {
                cout << "Сессия не найдена.\n";
            }
        } else if (choice == "5") {
            int id = readInt("Введите ID сессии для редактирования: ");
            StudySession* session = planner.findSession(id);
            if (!session) {
                cout << "Сессия не найдена.\n";
                continue;
            }
            cout << "Оставьте поле пустым, чтобы не менять.\n";
            string newSubj = readString("Предмет (" + session->subject + "): ");
            string newTopic = readString("Тема (" + session->topic + "): ");
            string newDt = readString("Дата/время (" + session->datetime + "): ");
            string newDur = readString("Длительность (" + to_string(session->duration) + "): ");
            string newPrio = readString("Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: " + priorityToString(session->priority) + ": ");
            map<string, string> updates;
            if (!newSubj.empty()) updates["subject"] = newSubj;
            if (!newTopic.empty()) updates["topic"] = newTopic;
            if (!newDt.empty()) updates["datetime"] = newDt;
            if (!newDur.empty()) updates["duration"] = newDur;
            if (!newPrio.empty()) {
                if (newPrio == "1") updates["priority"] = "Низкий";
                else if (newPrio == "3") updates["priority"] = "Высокий";
                else updates["priority"] = "Средний";
            }
            if (planner.updateSession(id, updates)) {
                cout << "Сессия обновлена.\n";
            } else {
                cout << "Ошибка обновления.\n";
            }
        } else if (choice == "6") {
            int id = readInt("Введите ID сессии для удаления: ");
            if (planner.deleteSession(id)) {
                cout << "Сессия удалена.\n";
            } else {
                cout << "Сессия не найдена.\n";
            }
        } else if (choice == "7") {
            auto stats = planner.getStats();
            cout << "\n=== СТАТИСТИКА ===\n";
            cout << "Всего сессий: " << get<int>(stats["total"]) << "\n";
            cout << "Выполнено: " << get<int>(stats["completed"]) << "\n";
            cout << "Запланировано: " << get<int>(stats["planned"]) << "\n";
            cout << "Пропущено: " << get<int>(stats["missed"]) << "\n";
            cout << "Общее время: " << get<int>(stats["total_time"]) << " мин\n";
            cout << "Время выполненных: " << get<int>(stats["completed_time"]) << " мин\n";
            cout << "Средняя длительность: " << fixed << setprecision(1) << get<double>(stats["avg_duration"]) << " мин\n";
            cout << "По предметам:\n";
            auto bySubject = get<map<string, int>>(stats["by_subject"]);
            for (const auto& [subj, mins] : bySubject) {
                cout << "  " << subj << ": " << mins << " мин\n";
            }
        } else if (choice == "8") {
            planner.saveToFile();
            cout << "Сохранено.\n";
        } else if (choice == "9") {
            planner.loadFromFile();
            cout << "Загружено.\n";
        } else {
            cout << "Неизвестная команда.\n";
        }
    }
    return 0;
}
