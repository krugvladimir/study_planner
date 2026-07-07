// study_planner.go
package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

type Priority string

const (
	LOW    Priority = "Низкий"
	MEDIUM Priority = "Средний"
	HIGH   Priority = "Высокий"
)

type SessionStatus string

const (
	PLANNED   SessionStatus = "Запланирована"
	COMPLETED SessionStatus = "Выполнена"
	MISSED    SessionStatus = "Пропущена"
)

type StudySession struct {
	ID       int            `json:"id"`
	Subject  string         `json:"subject"`
	Topic    string         `json:"topic"`
	Datetime string         `json:"datetime"`
	Duration int            `json:"duration"`
	Priority Priority       `json:"priority"`
	Status   SessionStatus  `json:"status"`
}

type PlannerData struct {
	Sessions []StudySession `json:"sessions"`
}

type StudyPlanner struct {
	sessions []StudySession
	nextID   int
}

func NewStudyPlanner() *StudyPlanner {
	return &StudyPlanner{
		sessions: []StudySession{},
		nextID:   1,
	}
}

func (p *StudyPlanner) AddSession(subject, topic, datetime string, duration int, priority Priority) (StudySession, error) {
	// Проверка формата даты
	_, err := time.Parse("2006-01-02 15:04", datetime)
	if err != nil {
		return StudySession{}, fmt.Errorf("неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ")
	}
	if duration <= 0 {
		return StudySession{}, fmt.Errorf("длительность должна быть положительной")
	}
	session := StudySession{
		ID:       p.nextID,
		Subject:  subject,
		Topic:    topic,
		Datetime: datetime,
		Duration: duration,
		Priority: priority,
		Status:   PLANNED,
	}
	p.sessions = append(p.sessions, session)
	p.nextID++
	return session, nil
}

func (p *StudyPlanner) FindSession(id int) *StudySession {
	for i := range p.sessions {
		if p.sessions[i].ID == id {
			return &p.sessions[i]
		}
	}
	return nil
}

func (p *StudyPlanner) UpdateSession(id int, updates map[string]interface{}) bool {
	session := p.FindSession(id)
	if session == nil {
		return false
	}
	for key, value := range updates {
		switch key {
		case "subject":
			if v, ok := value.(string); ok {
				session.Subject = v
			}
		case "topic":
			if v, ok := value.(string); ok {
				session.Topic = v
			}
		case "datetime":
			if v, ok := value.(string); ok {
				session.Datetime = v
			}
		case "duration":
			if v, ok := value.(int); ok {
				session.Duration = v
			}
		case "priority":
			if v, ok := value.(Priority); ok {
				session.Priority = v
			}
		case "status":
			if v, ok := value.(SessionStatus); ok {
				session.Status = v
			}
		}
	}
	return true
}

func (p *StudyPlanner) DeleteSession(id int) bool {
	for i, s := range p.sessions {
		if s.ID == id {
			p.sessions = append(p.sessions[:i], p.sessions[i+1:]...)
			return true
		}
	}
	return false
}

func (p *StudyPlanner) SetStatus(id int, status SessionStatus) bool {
	session := p.FindSession(id)
	if session == nil {
		return false
	}
	session.Status = status
	return true
}

func (p *StudyPlanner) FilterBySubject(subject string) []StudySession {
	var result []StudySession
	for _, s := range p.sessions {
		if strings.EqualFold(s.Subject, subject) {
			result = append(result, s)
		}
	}
	return result
}

func (p *StudyPlanner) FilterByStatus(status SessionStatus) []StudySession {
	var result []StudySession
	for _, s := range p.sessions {
		if s.Status == status {
			result = append(result, s)
		}
	}
	return result
}

func (p *StudyPlanner) FilterByDate(dateStr string) []StudySession {
	var result []StudySession
	for _, s := range p.sessions {
		if strings.HasPrefix(s.Datetime, dateStr) {
			result = append(result, s)
		}
	}
	return result
}

func (p *StudyPlanner) Stats() map[string]interface{} {
	total := len(p.sessions)
	completed := len(p.FilterByStatus(COMPLETED))
	planned := len(p.FilterByStatus(PLANNED))
	missed := len(p.FilterByStatus(MISSED))
	totalTime := 0
	completedTime := 0
	bySubject := make(map[string]int)
	for _, s := range p.sessions {
		totalTime += s.Duration
		if s.Status == COMPLETED {
			completedTime += s.Duration
		}
		bySubject[s.Subject] += s.Duration
	}
	avgDuration := 0.0
	if total > 0 {
		avgDuration = float64(totalTime) / float64(total)
	}
	return map[string]interface{}{
		"total":          total,
		"completed":      completed,
		"planned":        planned,
		"missed":         missed,
		"total_time":     totalTime,
		"completed_time": completedTime,
		"avg_duration":   avgDuration,
		"by_subject":     bySubject,
	}
}

func (p *StudyPlanner) SaveToFile(filename string) error {
	data := PlannerData{Sessions: p.sessions}
	jsonData, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filename, jsonData, 0644)
}

func (p *StudyPlanner) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	var pd PlannerData
	if err := json.Unmarshal(data, &pd); err != nil {
		return err
	}
	p.sessions = pd.Sessions
	for _, s := range p.sessions {
		if s.ID >= p.nextID {
			p.nextID = s.ID + 1
		}
	}
	return nil
}

func readString(prompt string) string {
	fmt.Print(prompt)
	reader := bufio.NewReader(os.Stdin)
	input, _ := reader.ReadString('\n')
	return strings.TrimSpace(input)
}

func readInt(prompt string) int {
	for {
		input := readString(prompt)
		if val, err := strconv.Atoi(input); err == nil {
			return val
		}
		fmt.Println("Введите число.")
	}
}

func printSession(session StudySession) {
	emoji := map[SessionStatus]string{PLANNED: "📅", COMPLETED: "✅", MISSED: "❌"}
	fmt.Printf("%s #%d - %s (%s)\n", emoji[session.Status], session.ID, session.Subject, session.Topic)
	fmt.Printf("   Дата/время: %s, Длительность: %d мин\n", session.Datetime, session.Duration)
	fmt.Printf("   Приоритет: %s, Статус: %s\n", session.Priority, session.Status)
}

func main() {
	planner := NewStudyPlanner()
	if err := planner.LoadFromFile("sessions_data.json"); err != nil {
		fmt.Println("Ошибка загрузки:", err)
	}

	for {
		fmt.Println("\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (Go) =====")
		fmt.Println("1. Добавить сессию")
		fmt.Println("2. Показать все сессии")
		fmt.Println("3. Показать сессии по предмету")
		fmt.Println("4. Отметить сессию как выполненную")
		fmt.Println("5. Редактировать сессию")
		fmt.Println("6. Удалить сессию")
		fmt.Println("7. Показать статистику")
		fmt.Println("8. Сохранить в файл")
		fmt.Println("9. Загрузить из файла")
		fmt.Println("0. Выход")
		choice := readString("Выберите действие: ")

		switch choice {
		case "0":
			return
		case "1":
			subject := readString("Предмет: ")
			if subject == "" {
				fmt.Println("Предмет не может быть пустым.")
				continue
			}
			topic := readString("Тема: ")
			datetime := readString("Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): ")
			duration := readInt("Длительность (мин): ")
			fmt.Println("Приоритет: 1-Низкий, 2-Средний, 3-Высокий")
			prioInput := readString("> ")
			var priority Priority
			switch prioInput {
			case "1":
				priority = LOW
			case "3":
				priority = HIGH
			default:
				priority = MEDIUM
			}
			session, err := planner.AddSession(subject, topic, datetime, duration, priority)
			if err != nil {
				fmt.Println("Ошибка:", err)
			} else {
				fmt.Printf("Сессия добавлена с ID %d\n", session.ID)
			}
		case "2":
			if len(planner.sessions) == 0 {
				fmt.Println("Нет сессий.")
			} else {
				for _, s := range planner.sessions {
					printSession(s)
				}
			}
		case "3":
			subject := readString("Введите предмет: ")
			sessions := planner.FilterBySubject(subject)
			if len(sessions) == 0 {
				fmt.Println("Сессий по этому предмету нет.")
			} else {
				for _, s := range sessions {
					printSession(s)
				}
			}
		case "4":
			id := readInt("Введите ID сессии: ")
			if planner.SetStatus(id, COMPLETED) {
				fmt.Println("Сессия отмечена как выполненная.")
			} else {
				fmt.Println("Сессия не найдена.")
			}
		case "5":
			id := readInt("Введите ID сессии для редактирования: ")
			session := planner.FindSession(id)
			if session == nil {
				fmt.Println("Сессия не найдена.")
				continue
			}
			fmt.Println("Оставьте поле пустым, чтобы не менять.")
			newSubj := readString(fmt.Sprintf("Предмет (%s): ", session.Subject))
			newTopic := readString(fmt.Sprintf("Тема (%s): ", session.Topic))
			newDt := readString(fmt.Sprintf("Дата/время (%s): ", session.Datetime))
			newDur := readString(fmt.Sprintf("Длительность (%d): ", session.Duration))
			newPrio := readString(fmt.Sprintf("Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: %s: ", session.Priority))
			updates := make(map[string]interface{})
			if newSubj != "" {
				updates["subject"] = newSubj
			}
			if newTopic != "" {
				updates["topic"] = newTopic
			}
			if newDt != "" {
				updates["datetime"] = newDt
			}
			if newDur != "" {
				if dur, err := strconv.Atoi(newDur); err == nil {
					updates["duration"] = dur
				} else {
					fmt.Println("Длительность должна быть числом, пропускаем.")
				}
			}
			if newPrio != "" {
				switch newPrio {
				case "1":
					updates["priority"] = LOW
				case "3":
					updates["priority"] = HIGH
				default:
					updates["priority"] = MEDIUM
				}
			}
			if planner.UpdateSession(id, updates) {
				fmt.Println("Сессия обновлена.")
			} else {
				fmt.Println("Ошибка обновления.")
			}
		case "6":
			id := readInt("Введите ID сессии для удаления: ")
			if planner.DeleteSession(id) {
				fmt.Println("Сессия удалена.")
			} else {
				fmt.Println("Сессия не найдена.")
			}
		case "7":
			stats := planner.Stats()
			fmt.Println("\n=== СТАТИСТИКА ===")
			fmt.Printf("Всего сессий: %d\n", stats["total"])
			fmt.Printf("Выполнено: %d\n", stats["completed"])
			fmt.Printf("Запланировано: %d\n", stats["planned"])
			fmt.Printf("Пропущено: %d\n", stats["missed"])
			fmt.Printf("Общее время: %d мин\n", stats["total_time"])
			fmt.Printf("Время выполненных: %d мин\n", stats["completed_time"])
			fmt.Printf("Средняя длительность: %.1f мин\n", stats["avg_duration"])
			fmt.Println("По предметам:")
			bySubject := stats["by_subject"].(map[string]int)
			for subj, mins := range bySubject {
				fmt.Printf("  %s: %d мин\n", subj, mins)
			}
		case "8":
			if err := planner.SaveToFile("sessions_data.json"); err != nil {
				fmt.Println("Ошибка сохранения:", err)
			} else {
				fmt.Println("Сохранено.")
			}
		case "9":
			if err := planner.LoadFromFile("sessions_data.json"); err != nil {
				fmt.Println("Ошибка загрузки:", err)
			} else {
				fmt.Println("Загружено.")
			}
		default:
			fmt.Println("Неизвестная команда.")
		}
	}
}
