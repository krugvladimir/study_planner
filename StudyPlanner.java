// StudyPlanner.java
import java.io.*;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.stream.Collectors;

enum Priority { НИЗКИЙ, СРЕДНИЙ, ВЫСОКИЙ }
enum SessionStatus { ЗАПЛАНИРОВАНА, ВЫПОЛНЕНА, ПРОПУЩЕНА }

record StudySession(int id, String subject, String topic, String datetime, int duration, Priority priority, SessionStatus status) implements Serializable {}

class PlannerData implements Serializable {
    private static final long serialVersionUID = 1L;
    public List<StudySession> sessions;
}

class StudyPlanner implements Serializable {
    private static final long serialVersionUID = 1L;
    private List<StudySession> sessions = new ArrayList<>();
    private int nextId = 1;

    private void validateDatetime(String dt) {
        try {
            LocalDateTime.parse(dt, DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm"));
        } catch (Exception e) {
            throw new IllegalArgumentException("Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ");
        }
    }

    public StudySession addSession(String subject, String topic, String datetime, int duration, Priority priority) {
        validateDatetime(datetime);
        if (duration <= 0) throw new IllegalArgumentException("Длительность должна быть положительной");
        StudySession session = new StudySession(nextId, subject, topic, datetime, duration, priority, SessionStatus.ЗАПЛАНИРОВАНА);
        sessions.add(session);
        nextId++;
        return session;
    }

    public Optional<StudySession> findSession(int id) {
        return sessions.stream().filter(s -> s.id() == id).findFirst();
    }

    public boolean updateSession(int id, Map<String, Object> updates) {
        Optional<StudySession> opt = findSession(id);
        if (opt.isEmpty()) return false;
        StudySession old = opt.get();
        sessions.remove(old);
        String subject = (String) updates.getOrDefault("subject", old.subject());
        String topic = (String) updates.getOrDefault("topic", old.topic());
        String datetime = (String) updates.getOrDefault("datetime", old.datetime());
        int duration = (int) updates.getOrDefault("duration", old.duration());
        Priority priority = (Priority) updates.getOrDefault("priority", old.priority());
        SessionStatus status = (SessionStatus) updates.getOrDefault("status", old.status());
        StudySession updated = new StudySession(old.id(), subject, topic, datetime, duration, priority, status);
        sessions.add(updated);
        return true;
    }

    public boolean deleteSession(int id) {
        return sessions.removeIf(s -> s.id() == id);
    }

    public boolean setStatus(int id, SessionStatus status) {
        Optional<StudySession> opt = findSession(id);
        if (opt.isEmpty()) return false;
        StudySession old = opt.get();
        sessions.remove(old);
        StudySession updated = new StudySession(old.id(), old.subject(), old.topic(), old.datetime(), old.duration(), old.priority(), status);
        sessions.add(updated);
        return true;
    }

    public List<StudySession> filterBySubject(String subject) {
        return sessions.stream().filter(s -> s.subject().equalsIgnoreCase(subject)).collect(Collectors.toList());
    }

    public List<StudySession> filterByStatus(SessionStatus status) {
        return sessions.stream().filter(s -> s.status() == status).collect(Collectors.toList());
    }

    public List<StudySession> filterByDate(String dateStr) {
        return sessions.stream().filter(s -> s.datetime().startsWith(dateStr)).collect(Collectors.toList());
    }

    public Map<String, Object> getStats() {
        int total = sessions.size();
        int completed = filterByStatus(SessionStatus.ВЫПОЛНЕНА).size();
        int planned = filterByStatus(SessionStatus.ЗАПЛАНИРОВАНА).size();
        int missed = filterByStatus(SessionStatus.ПРОПУЩЕНА).size();
        int totalTime = sessions.stream().mapToInt(StudySession::duration).sum();
        int completedTime = filterByStatus(SessionStatus.ВЫПОЛНЕНА).stream().mapToInt(StudySession::duration).sum();
        double avgDuration = total > 0 ? (double) totalTime / total : 0;
        Map<String, Integer> bySubject = new HashMap<>();
        for (StudySession s : sessions) {
            bySubject.put(s.subject(), bySubject.getOrDefault(s.subject(), 0) + s.duration());
        }
        Map<String, Object> stats = new HashMap<>();
        stats.put("total", total);
        stats.put("completed", completed);
        stats.put("planned", planned);
        stats.put("missed", missed);
        stats.put("total_time", totalTime);
        stats.put("completed_time", completedTime);
        stats.put("avg_duration", avgDuration);
        stats.put("by_subject", bySubject);
        return stats;
    }

    public void saveToFile(String filename) throws IOException {
        PlannerData data = new PlannerData();
        data.sessions = new ArrayList<>(sessions);
        try (ObjectOutputStream oos = new ObjectOutputStream(Files.newOutputStream(Paths.get(filename)))) {
            oos.writeObject(data);
        }
    }

    public void loadFromFile(String filename) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(Files.newInputStream(Paths.get(filename)))) {
            PlannerData data = (PlannerData) ois.readObject();
            sessions = new ArrayList<>(data.sessions);
            for (StudySession s : sessions) {
                if (s.id() >= nextId) nextId = s.id() + 1;
            }
        }
    }

    public List<StudySession> getSessions() { return Collections.unmodifiableList(sessions); }
}

public class StudyPlannerApp {
    private static final Scanner scanner = new Scanner(System.in);

    private static String readString(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private static int readInt(String prompt) {
        while (true) {
            try {
                System.out.print(prompt);
                return Integer.parseInt(scanner.nextLine().trim());
            } catch (NumberFormatException e) {
                System.out.println("Введите число.");
            }
        }
    }

    private static void printSession(StudySession session) {
        String emoji = switch (session.status()) {
            case ЗАПЛАНИРОВАНА -> "📅";
            case ВЫПОЛНЕНА -> "✅";
            case ПРОПУЩЕНА -> "❌";
        };
        System.out.printf("%s #%d - %s (%s)%n", emoji, session.id(), session.subject(), session.topic());
        System.out.printf("   Дата/время: %s, Длительность: %d мин%n", session.datetime(), session.duration());
        System.out.printf("   Приоритет: %s, Статус: %s%n", session.priority(), session.status());
    }

    public static void main(String[] args) {
        StudyPlanner planner = new StudyPlanner();
        try {
            planner.loadFromFile("sessions_data.ser");
        } catch (IOException | ClassNotFoundException e) {
            System.out.println("Не удалось загрузить данные.");
        }

        while (true) {
            System.out.println("\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (Java) =====");
            System.out.println("1. Добавить сессию");
            System.out.println("2. Показать все сессии");
            System.out.println("3. Показать сессии по предмету");
            System.out.println("4. Отметить сессию как выполненную");
            System.out.println("5. Редактировать сессию");
            System.out.println("6. Удалить сессию");
            System.out.println("7. Показать статистику");
            System.out.println("8. Сохранить в файл");
            System.out.println("9. Загрузить из файла");
            System.out.println("0. Выход");
            String choice = readString("Выберите действие: ");

            switch (choice) {
                case "0" -> { return; }
                case "1" -> {
                    String subject = readString("Предмет: ");
                    if (subject.isBlank()) {
                        System.out.println("Предмет не может быть пустым.");
                        continue;
                    }
                    String topic = readString("Тема: ");
                    String datetime = readString("Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ");
                    int duration = readInt("Длительность (мин): ");
                    System.out.println("Приоритет: 1-Низкий, 2-Средний, 3-Высокий");
                    String prioInput = readString("> ");
                    Priority priority;
                    if (prioInput.equals("1")) priority = Priority.НИЗКИЙ;
                    else if (prioInput.equals("3")) priority = Priority.ВЫСОКИЙ;
                    else priority = Priority.СРЕДНИЙ;
                    try {
                        StudySession session = planner.addSession(subject, topic, datetime, duration, priority);
                        System.out.println("Сессия добавлена с ID " + session.id());
                    } catch (Exception e) {
                        System.out.println("Ошибка: " + e.getMessage());
                    }
                }
                case "2" -> {
                    if (planner.getSessions().isEmpty()) System.out.println("Нет сессий.");
                    else planner.getSessions().forEach(StudyPlannerApp::printSession);
                }
                case "3" -> {
                    String subject = readString("Введите предмет: ");
                    var sessions = planner.filterBySubject(subject);
                    if (sessions.isEmpty()) System.out.println("Сессий по этому предмету нет.");
                    else sessions.forEach(StudyPlannerApp::printSession);
                }
                case "4" -> {
                    int id = readInt("Введите ID сессии: ");
                    if (planner.setStatus(id, SessionStatus.ВЫПОЛНЕНА)) {
                        System.out.println("Сессия отмечена как выполненная.");
                    } else {
                        System.out.println("Сессия не найдена.");
                    }
                }
                case "5" -> {
                    int id = readInt("Введите ID сессии для редактирования: ");
                    var opt = planner.findSession(id);
                    if (opt.isEmpty()) {
                        System.out.println("Сессия не найдена.");
                        continue;
                    }
                    StudySession old = opt.get();
                    System.out.println("Оставьте поле пустым, чтобы не менять.");
                    String newSubj = readString("Предмет (" + old.subject() + "): ");
                    String newTopic = readString("Тема (" + old.topic() + "): ");
                    String newDt = readString("Дата/время (" + old.datetime() + "): ");
                    String newDurStr = readString("Длительность (" + old.duration() + "): ");
                    String newPrioStr = readString("Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: " + old.priority() + ": ");
                    Map<String, Object> updates = new HashMap<>();
                    if (!newSubj.isBlank()) updates.put("subject", newSubj);
                    if (!newTopic.isBlank()) updates.put("topic", newTopic);
                    if (!newDt.isBlank()) updates.put("datetime", newDt);
                    if (!newDurStr.isBlank()) {
                        try {
                            updates.put("duration", Integer.parseInt(newDurStr));
                        } catch (NumberFormatException e) {
                            System.out.println("Длительность должна быть числом, пропускаем.");
                        }
                    }
                    if (!newPrioStr.isBlank()) {
                        if (newPrioStr.equals("1")) updates.put("priority", Priority.НИЗКИЙ);
                        else if (newPrioStr.equals("3")) updates.put("priority", Priority.ВЫСОКИЙ);
                        else updates.put("priority", Priority.СРЕДНИЙ);
                    }
                    if (planner.updateSession(id, updates)) {
                        System.out.println("Сессия обновлена.");
                    } else {
                        System.out.println("Ошибка обновления.");
                    }
                }
                case "6" -> {
                    int id = readInt("Введите ID сессии для удаления: ");
                    if (planner.deleteSession(id)) {
                        System.out.println("Сессия удалена.");
                    } else {
                        System.out.println("Сессия не найдена.");
                    }
                }
                case "7" -> {
                    var stats = planner.getStats();
                    System.out.println("\n=== СТАТИСТИКА ===");
                    System.out.println("Всего сессий: " + stats.get("total"));
                    System.out.println("Выполнено: " + stats.get("completed"));
                    System.out.println("Запланировано: " + stats.get("planned"));
                    System.out.println("Пропущено: " + stats.get("missed"));
                    System.out.println("Общее время: " + stats.get("total_time") + " мин");
                    System.out.println("Время выполненных: " + stats.get("completed_time") + " мин");
                    System.out.printf("Средняя длительность: %.1f мин%n", stats.get("avg_duration"));
                    System.out.println("По предметам:");
                    @SuppressWarnings("unchecked")
                    Map<String, Integer> bySubject = (Map<String, Integer>) stats.get("by_subject");
                    for (var entry : bySubject.entrySet()) {
                        System.out.println("  " + entry.getKey() + ": " + entry.getValue() + " мин");
                    }
                }
                case "8" -> {
                    try {
                        planner.saveToFile("sessions_data.ser");
                        System.out.println("Сохранено.");
                    } catch (IOException e) {
                        System.out.println("Ошибка сохранения: " + e.getMessage());
                    }
                }
                case "9" -> {
                    try {
                        planner.loadFromFile("sessions_data.ser");
                        System.out.println("Загружено.");
                    } catch (IOException | ClassNotFoundException e) {
                        System.out.println("Ошибка загрузки: " + e.getMessage());
                    }
                }
                default -> System.out.println("Неизвестная команда.");
            }
        }
    }
}
