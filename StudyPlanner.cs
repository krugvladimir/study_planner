// StudyPlanner.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

public enum Priority { Низкий, Средний, Высокий }
public enum SessionStatus { Запланирована, Выполнена, Пропущена }

public record StudySession(
    int Id,
    string Subject,
    string Topic,
    string Datetime,
    int Duration,
    Priority Priority,
    SessionStatus Status
);

public class PlannerData
{
    public List<StudySession> Sessions { get; set; } = new();
}

public class StudyPlanner
{
    private List<StudySession> sessions = new();
    private int nextId = 1;

    public IReadOnlyList<StudySession> Sessions => sessions.AsReadOnly();

    private void ValidateDatetime(string dt)
    {
        if (!DateTime.TryParseExact(dt, "yyyy-MM-dd HH:mm", null, System.Globalization.DateTimeStyles.None, out _))
            throw new ArgumentException("Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ");
    }

    public StudySession AddSession(string subject, string topic, string datetime, int duration, Priority priority)
    {
        ValidateDatetime(datetime);
        if (duration <= 0) throw new ArgumentException("Длительность должна быть положительной");
        var session = new StudySession(nextId, subject, topic, datetime, duration, priority, SessionStatus.Запланирована);
        sessions.Add(session);
        nextId++;
        return session;
    }

    public StudySession? FindSession(int id) => sessions.FirstOrDefault(s => s.Id == id);

    public bool UpdateSession(int id, Dictionary<string, object> updates)
    {
        var old = FindSession(id);
        if (old == null) return false;
        sessions.Remove(old);
        string subject = updates.ContainsKey("subject") ? (string)updates["subject"] : old.Subject;
        string topic = updates.ContainsKey("topic") ? (string)updates["topic"] : old.Topic;
        string datetime = updates.ContainsKey("datetime") ? (string)updates["datetime"] : old.Datetime;
        int duration = updates.ContainsKey("duration") ? (int)updates["duration"] : old.Duration;
        Priority priority = updates.ContainsKey("priority") ? (Priority)updates["priority"] : old.Priority;
        SessionStatus status = updates.ContainsKey("status") ? (SessionStatus)updates["status"] : old.Status;
        var updated = new StudySession(old.Id, subject, topic, datetime, duration, priority, status);
        sessions.Add(updated);
        return true;
    }

    public bool DeleteSession(int id) => sessions.RemoveAll(s => s.Id == id) > 0;

    public bool SetStatus(int id, SessionStatus status)
    {
        var old = FindSession(id);
        if (old == null) return false;
        sessions.Remove(old);
        var updated = old with { Status = status };
        sessions.Add(updated);
        return true;
    }

    public List<StudySession> FilterBySubject(string subject)
        => sessions.Where(s => string.Equals(s.Subject, subject, StringComparison.OrdinalIgnoreCase)).ToList();

    public List<StudySession> FilterByStatus(SessionStatus status)
        => sessions.Where(s => s.Status == status).ToList();

    public List<StudySession> FilterByDate(string dateStr)
        => sessions.Where(s => s.Datetime.StartsWith(dateStr)).ToList();

    public Dictionary<string, object> GetStats()
    {
        int total = sessions.Count;
        int completed = FilterByStatus(SessionStatus.Выполнена).Count;
        int planned = FilterByStatus(SessionStatus.Запланирована).Count;
        int missed = FilterByStatus(SessionStatus.Пропущена).Count;
        int totalTime = sessions.Sum(s => s.Duration);
        int completedTime = FilterByStatus(SessionStatus.Выполнена).Sum(s => s.Duration);
        double avgDuration = total > 0 ? (double)totalTime / total : 0;
        var bySubject = sessions.GroupBy(s => s.Subject)
                                .ToDictionary(g => g.Key, g => g.Sum(s => s.Duration));
        return new Dictionary<string, object>
        {
            ["total"] = total,
            ["completed"] = completed,
            ["planned"] = planned,
            ["missed"] = missed,
            ["total_time"] = totalTime,
            ["completed_time"] = completedTime,
            ["avg_duration"] = avgDuration,
            ["by_subject"] = bySubject
        };
    }

    public void SaveToFile(string filename)
    {
        var data = new PlannerData { Sessions = sessions };
        var options = new JsonSerializerOptions { WriteIndented = true };
        string json = JsonSerializer.Serialize(data, options);
        File.WriteAllText(filename, json);
    }

    public void LoadFromFile(string filename)
    {
        if (!File.Exists(filename)) return;
        string json = File.ReadAllText(filename);
        var data = JsonSerializer.Deserialize<PlannerData>(json);
        if (data != null)
        {
            sessions = data.Sessions;
            nextId = sessions.Any() ? sessions.Max(s => s.Id) + 1 : 1;
        }
    }
}

public static class Program
{
    private static string ReadString(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private static int ReadInt(string prompt)
    {
        while (true)
        {
            Console.Write(prompt);
            if (int.TryParse(Console.ReadLine(), out int result))
                return result;
            Console.WriteLine("Введите число.");
        }
    }

    private static void PrintSession(StudySession session)
    {
        string emoji = session.Status switch
        {
            SessionStatus.Запланирована => "📅",
            SessionStatus.Выполнена => "✅",
            SessionStatus.Пропущена => "❌",
            _ => ""
        };
        Console.WriteLine($"{emoji} #{session.Id} - {session.Subject} ({session.Topic})");
        Console.WriteLine($"   Дата/время: {session.Datetime}, Длительность: {session.Duration} мин");
        Console.WriteLine($"   Приоритет: {session.Priority}, Статус: {session.Status}");
    }

    public static void Main()
    {
        var planner = new StudyPlanner();
        try { planner.LoadFromFile("sessions_data.json"); }
        catch { Console.WriteLine("Не удалось загрузить данные."); }

        while (true)
        {
            Console.WriteLine("\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (C#) =====");
            Console.WriteLine("1. Добавить сессию");
            Console.WriteLine("2. Показать все сессии");
            Console.WriteLine("3. Показать сессии по предмету");
            Console.WriteLine("4. Отметить сессию как выполненную");
            Console.WriteLine("5. Редактировать сессию");
            Console.WriteLine("6. Удалить сессию");
            Console.WriteLine("7. Показать статистику");
            Console.WriteLine("8. Сохранить в файл");
            Console.WriteLine("9. Загрузить из файла");
            Console.WriteLine("0. Выход");
            string choice = ReadString("Выберите действие: ");

            switch (choice)
            {
                case "0": return;
                case "1":
                    string subject = ReadString("Предмет: ");
                    if (string.IsNullOrWhiteSpace(subject)) { Console.WriteLine("Предмет не может быть пустым."); continue; }
                    string topic = ReadString("Тема: ");
                    string datetime = ReadString("Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ");
                    int duration = ReadInt("Длительность (мин): ");
                    Console.WriteLine("Приоритет: 1-Низкий, 2-Средний, 3-Высокий");
                    string prioInput = ReadString("> ");
                    Priority priority = prioInput switch
                    {
                        "1" => Priority.Низкий,
                        "3" => Priority.Высокий,
                        _ => Priority.Средний
                    };
                    try
                    {
                        var session = planner.AddSession(subject, topic, datetime, duration, priority);
                        Console.WriteLine($"Сессия добавлена с ID {session.Id}");
                    }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "2":
                    if (!planner.Sessions.Any()) Console.WriteLine("Нет сессий.");
                    else foreach (var s in planner.Sessions) PrintSession(s);
                    break;
                case "3":
                    string subj = ReadString("Введите предмет: ");
                    var filtered = planner.FilterBySubject(subj);
                    if (!filtered.Any()) Console.WriteLine("Сессий по этому предмету нет.");
                    else filtered.ForEach(PrintSession);
                    break;
                case "4":
                    int id = ReadInt("Введите ID сессии: ");
                    if (planner.SetStatus(id, SessionStatus.Выполнена))
                        Console.WriteLine("Сессия отмечена как выполненная.");
                    else
                        Console.WriteLine("Сессия не найдена.");
                    break;
                case "5":
                    int eid = ReadInt("Введите ID сессии для редактирования: ");
                    var old = planner.FindSession(eid);
                    if (old == null) { Console.WriteLine("Сессия не найдена."); continue; }
                    Console.WriteLine("Оставьте поле пустым, чтобы не менять.");
                    string newSubj = ReadString($"Предмет ({old.Subject}): ");
                    string newTopic = ReadString($"Тема ({old.Topic}): ");
                    string newDt = ReadString($"Дата/время ({old.Datetime}): ");
                    string newDurStr = ReadString($"Длительность ({old.Duration}): ");
                    string newPrioStr = ReadString($"Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: {old.Priority}: ");
                    var updates = new Dictionary<string, object>();
                    if (!string.IsNullOrWhiteSpace(newSubj)) updates["subject"] = newSubj;
                    if (!string.IsNullOrWhiteSpace(newTopic)) updates["topic"] = newTopic;
                    if (!string.IsNullOrWhiteSpace(newDt)) updates["datetime"] = newDt;
                    if (!string.IsNullOrWhiteSpace(newDurStr))
                    {
                        if (int.TryParse(newDurStr, out int dur)) updates["duration"] = dur;
                        else Console.WriteLine("Длительность должна быть числом, пропускаем.");
                    }
                    if (!string.IsNullOrWhiteSpace(newPrioStr))
                    {
                        updates["priority"] = newPrioStr switch
                        {
                            "1" => Priority.Низкий,
                            "3" => Priority.Высокий,
                            _ => Priority.Средний
                        };
                    }
                    if (planner.UpdateSession(eid, updates)) Console.WriteLine("Сессия обновлена.");
                    else Console.WriteLine("Ошибка обновления.");
                    break;
                case "6":
                    int did = ReadInt("Введите ID сессии для удаления: ");
                    if (planner.DeleteSession(did)) Console.WriteLine("Сессия удалена.");
                    else Console.WriteLine("Сессия не найдена.");
                    break;
                case "7":
                    var stats = planner.GetStats();
                    Console.WriteLine("\n=== СТАТИСТИКА ===");
                    Console.WriteLine($"Всего сессий: {stats["total"]}");
                    Console.WriteLine($"Выполнено: {stats["completed"]}");
                    Console.WriteLine($"Запланировано: {stats["planned"]}");
                    Console.WriteLine($"Пропущено: {stats["missed"]}");
                    Console.WriteLine($"Общее время: {stats["total_time"]} мин");
                    Console.WriteLine($"Время выполненных: {stats["completed_time"]} мин");
                    Console.WriteLine($"Средняя длительность: {stats["avg_duration"]:F1} мин");
                    Console.WriteLine("По предметам:");
                    var bySubject = (Dictionary<string, int>)stats["by_subject"];
                    foreach (var kv in bySubject)
                        Console.WriteLine($"  {kv.Key}: {kv.Value} мин");
                    break;
                case "8":
                    try { planner.SaveToFile("sessions_data.json"); Console.WriteLine("Сохранено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "9":
                    try { planner.LoadFromFile("sessions_data.json"); Console.WriteLine("Загружено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                default: Console.WriteLine("Неизвестная команда."); break;
            }
        }
    }
}
