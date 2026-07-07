# study_planner.py
import json
from dataclasses import dataclass, asdict
from enum import Enum
from datetime import datetime, timedelta
from typing import List, Optional
from pathlib import Path

class Priority(Enum):
    LOW = "Низкий"
    MEDIUM = "Средний"
    HIGH = "Высокий"

class SessionStatus(Enum):
    PLANNED = "Запланирована"
    COMPLETED = "Выполнена"
    MISSED = "Пропущена"

@dataclass
class StudySession:
    id: int
    subject: str
    topic: str
    datetime: str  # "YYYY-MM-DD HH:MM"
    duration: int   # minutes
    priority: Priority
    status: SessionStatus

class StudyPlanner:
    def __init__(self):
        self.sessions: List[StudySession] = []
        self.next_id = 1

    def add_session(self, subject: str, topic: str, dt: str, duration: int, priority: Priority) -> StudySession:
        # Проверка формата даты
        try:
            datetime.strptime(dt, "%Y-%m-%d %H:%M")
        except ValueError:
            raise ValueError("Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ")
        if duration <= 0:
            raise ValueError("Длительность должна быть положительной")
        session = StudySession(
            id=self.next_id,
            subject=subject,
            topic=topic,
            datetime=dt,
            duration=duration,
            priority=priority,
            status=SessionStatus.PLANNED
        )
        self.sessions.append(session)
        self.next_id += 1
        return session

    def find_session(self, session_id: int) -> Optional[StudySession]:
        return next((s for s in self.sessions if s.id == session_id), None)

    def update_session(self, session_id: int, **kwargs) -> bool:
        session = self.find_session(session_id)
        if not session:
            return False
        for key, value in kwargs.items():
            if hasattr(session, key) and value is not None:
                setattr(session, key, value)
        return True

    def delete_session(self, session_id: int) -> bool:
        session = self.find_session(session_id)
        if session:
            self.sessions.remove(session)
            return True
        return False

    def set_status(self, session_id: int, status: SessionStatus) -> bool:
        session = self.find_session(session_id)
        if not session:
            return False
        session.status = status
        return True

    def filter_by_subject(self, subject: str) -> List[StudySession]:
        return [s for s in self.sessions if s.subject.lower() == subject.lower()]

    def filter_by_status(self, status: SessionStatus) -> List[StudySession]:
        return [s for s in self.sessions if s.status == status]

    def filter_by_date(self, date_str: str) -> List[StudySession]:
        return [s for s in self.sessions if s.datetime.startswith(date_str)]

    def get_stats(self) -> dict:
        total = len(self.sessions)
        completed = len(self.filter_by_status(SessionStatus.COMPLETED))
        planned = len(self.filter_by_status(SessionStatus.PLANNED))
        missed = len(self.filter_by_status(SessionStatus.MISSED))
        total_time = sum(s.duration for s in self.sessions)
        completed_time = sum(s.duration for s in self.sessions if s.status == SessionStatus.COMPLETED)
        # Статистика по предметам
        subjects = {}
        for s in self.sessions:
            subjects[s.subject] = subjects.get(s.subject, 0) + s.duration
        avg_duration = total_time / total if total > 0 else 0
        return {
            "total": total,
            "completed": completed,
            "planned": planned,
            "missed": missed,
            "total_time": total_time,
            "completed_time": completed_time,
            "avg_duration": avg_duration,
            "by_subject": subjects
        }

    def save_to_file(self, filename: str = "sessions_data.json") -> None:
        data = {"sessions": [asdict(s) for s in self.sessions]}
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

    def load_from_file(self, filename: str = "sessions_data.json") -> None:
        path = Path(filename)
        if not path.exists():
            return
        with open(filename, "r", encoding="utf-8") as f:
            data = json.load(f)
            self.sessions.clear()
            for item in data.get("sessions", []):
                session = StudySession(
                    id=item["id"],
                    subject=item["subject"],
                    topic=item["topic"],
                    datetime=item["datetime"],
                    duration=item["duration"],
                    priority=Priority(item["priority"]),
                    status=SessionStatus(item["status"])
                )
                self.sessions.append(session)
                if session.id >= self.next_id:
                    self.next_id = session.id + 1

def print_session(session: StudySession) -> None:
    status_emoji = {"Запланирована": "📅", "Выполнена": "✅", "Пропущена": "❌"}
    print(f"{status_emoji[session.status.value]} #{session.id} - {session.subject} ({session.topic})")
    print(f"   Дата/время: {session.datetime}, Длительность: {session.duration} мин")
    print(f"   Приоритет: {session.priority.value}, Статус: {session.status.value}")

def main():
    planner = StudyPlanner()
    planner.load_from_file()

    while True:
        print("\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (Python) =====")
        print("1. Добавить сессию")
        print("2. Показать все сессии")
        print("3. Показать сессии по предмету")
        print("4. Отметить сессию как выполненную")
        print("5. Редактировать сессию")
        print("6. Удалить сессию")
        print("7. Показать статистику")
        print("8. Сохранить в файл")
        print("9. Загрузить из файла")
        print("0. Выход")
        choice = input("Выберите действие: ").strip()

        if choice == "0":
            break
        elif choice == "1":
            subject = input("Предмет: ").strip()
            if not subject:
                print("Предмет не может быть пустым.")
                continue
            topic = input("Тема: ").strip()
            dt = input("Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ").strip()
            try:
                dur = int(input("Длительность (мин): ").strip())
            except ValueError:
                print("Введите число.")
                continue
            print("Приоритет: 1-Низкий, 2-Средний, 3-Высокий")
            prio_input = input("> ").strip()
            if prio_input == "1":
                priority = Priority.LOW
            elif prio_input == "3":
                priority = Priority.HIGH
            else:
                priority = Priority.MEDIUM
            try:
                session = planner.add_session(subject, topic, dt, dur, priority)
                print(f"Сессия добавлена с ID {session.id}")
            except Exception as e:
                print("Ошибка:", e)
        elif choice == "2":
            if not planner.sessions:
                print("Нет сессий.")
            else:
                for s in planner.sessions:
                    print_session(s)
        elif choice == "3":
            subject = input("Введите предмет: ").strip()
            sessions = planner.filter_by_subject(subject)
            if not sessions:
                print("Сессий по этому предмету нет.")
            else:
                for s in sessions:
                    print_session(s)
        elif choice == "4":
            try:
                sid = int(input("Введите ID сессии: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if planner.set_status(sid, SessionStatus.COMPLETED):
                print("Сессия отмечена как выполненная.")
            else:
                print("Сессия не найдена.")
        elif choice == "5":
            try:
                sid = int(input("Введите ID сессии для редактирования: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            session = planner.find_session(sid)
            if not session:
                print("Сессия не найдена.")
                continue
            print("Оставьте поле пустым, чтобы не менять.")
            new_subj = input(f"Предмет ({session.subject}): ").strip()
            new_topic = input(f"Тема ({session.topic}): ").strip()
            new_dt = input(f"Дата/время ({session.datetime}): ").strip()
            new_dur = input(f"Длительность ({session.duration}): ").strip()
            new_prio = input(f"Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: {session.priority.value}: ").strip()
            updates = {}
            if new_subj: updates["subject"] = new_subj
            if new_topic: updates["topic"] = new_topic
            if new_dt: updates["datetime"] = new_dt
            if new_dur:
                try:
                    updates["duration"] = int(new_dur)
                except ValueError:
                    print("Длительность должна быть числом, пропускаем.")
            if new_prio:
                if new_prio == "1": updates["priority"] = Priority.LOW
                elif new_prio == "3": updates["priority"] = Priority.HIGH
                else: updates["priority"] = Priority.MEDIUM
            if planner.update_session(sid, **updates):
                print("Сессия обновлена.")
            else:
                print("Ошибка обновления.")
        elif choice == "6":
            try:
                sid = int(input("Введите ID сессии для удаления: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if planner.delete_session(sid):
                print("Сессия удалена.")
            else:
                print("Сессия не найдена.")
        elif choice == "7":
            stats = planner.get_stats()
            print("\n=== СТАТИСТИКА ===")
            print(f"Всего сессий: {stats['total']}")
            print(f"Выполнено: {stats['completed']}")
            print(f"Запланировано: {stats['planned']}")
            print(f"Пропущено: {stats['missed']}")
            print(f"Общее время: {stats['total_time']} мин")
            print(f"Время выполненных: {stats['completed_time']} мин")
            print(f"Средняя длительность: {stats['avg_duration']:.1f} мин")
            print("По предметам:")
            for subj, minutes in stats['by_subject'].items():
                print(f"  {subj}: {minutes} мин")
        elif choice == "8":
            planner.save_to_file()
            print("Сохранено.")
        elif choice == "9":
            planner.load_from_file()
            print("Загружено.")
        else:
            print("Неизвестная команда.")

if __name__ == "__main__":
    main()
