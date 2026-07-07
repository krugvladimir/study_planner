# study_planner.rb
require 'json'
require 'date'

class Priority
  LOW = "Низкий"
  MEDIUM = "Средний"
  HIGH = "Высокий"
end

class SessionStatus
  PLANNED = "Запланирована"
  COMPLETED = "Выполнена"
  MISSED = "Пропущена"
end

class StudySession
  attr_accessor :id, :subject, :topic, :datetime, :duration, :priority, :status

  def initialize(id, subject, topic, datetime, duration, priority = Priority::MEDIUM, status = SessionStatus::PLANNED)
    @id = id
    @subject = subject
    @topic = topic
    @datetime = datetime
    @duration = duration
    @priority = priority
    @status = status
  end

  def to_h
    {
      id: @id,
      subject: @subject,
      topic: @topic,
      datetime: @datetime,
      duration: @duration,
      priority: @priority,
      status: @status
    }
  end

  def self.from_h(hash)
    StudySession.new(hash[:id], hash[:subject], hash[:topic], hash[:datetime],
                     hash[:duration], hash[:priority], hash[:status])
  end
end

class StudyPlanner
  attr_reader :sessions

  def initialize
    @sessions = []
    @next_id = 1
  end

  def add_session(subject, topic, datetime, duration, priority = Priority::MEDIUM)
    # Проверка формата даты
    begin
      DateTime.strptime(datetime, "%Y-%m-%d %H:%M")
    rescue ArgumentError
      raise "Неверный формат даты/времени, используйте ГГГГ-ММ-ДД ЧЧ:ММ"
    end
    raise "Длительность должна быть положительной" if duration <= 0
    session = StudySession.new(@next_id, subject, topic, datetime, duration, priority)
    @sessions << session
    @next_id += 1
    session
  end

  def find_session(id)
    @sessions.find { |s| s.id == id }
  end

  def update_session(id, **kwargs)
    session = find_session(id)
    return false unless session
    kwargs.each do |key, value|
      session.send("#{key}=", value) if session.respond_to?("#{key}=")
    end
    true
  end

  def delete_session(id)
    session = find_session(id)
    return false unless session
    @sessions.delete(session)
    true
  end

  def set_status(id, status)
    session = find_session(id)
    return false unless session
    session.status = status
    true
  end

  def filter_by_subject(subject)
    @sessions.select { |s| s.subject.downcase == subject.downcase }
  end

  def filter_by_status(status)
    @sessions.select { |s| s.status == status }
  end

  def filter_by_date(date_str)
    @sessions.select { |s| s.datetime.start_with?(date_str) }
  end

  def stats
    total = @sessions.size
    completed = filter_by_status(SessionStatus::COMPLETED).size
    planned = filter_by_status(SessionStatus::PLANNED).size
    missed = filter_by_status(SessionStatus::MISSED).size
    total_time = @sessions.sum(&:duration)
    completed_time = filter_by_status(SessionStatus::COMPLETED).sum(&:duration)
    avg_duration = total > 0 ? total_time.to_f / total : 0
    by_subject = Hash.new(0)
    @sessions.each { |s| by_subject[s.subject] += s.duration }
    {
      total: total,
      completed: completed,
      planned: planned,
      missed: missed,
      total_time: total_time,
      completed_time: completed_time,
      avg_duration: avg_duration,
      by_subject: by_subject
    }
  end

  def save_to_file(filename = "sessions_data.json")
    data = { sessions: @sessions.map(&:to_h) }
    File.write(filename, JSON.pretty_generate(data))
  end

  def load_from_file(filename = "sessions_data.json")
    return unless File.exist?(filename)
    data = JSON.parse(File.read(filename), symbolize_names: true)
    @sessions.clear
    data[:sessions].each do |item|
      session = StudySession.from_h(item)
      @sessions << session
      @next_id = session.id + 1 if session.id >= @next_id
    end
  rescue JSON::ParserError
    puts "Ошибка чтения файла."
  end
end

def print_session(session)
  emoji = { "Запланирована" => "📅", "Выполнена" => "✅", "Пропущена" => "❌" }
  puts "#{emoji[session.status]} ##{session.id} - #{session.subject} (#{session.topic})"
  puts "   Дата/время: #{session.datetime}, Длительность: #{session.duration} мин"
  puts "   Приоритет: #{session.priority}, Статус: #{session.status}"
end

def main
  planner = StudyPlanner.new
  planner.load_from_file

  loop do
    puts "\n===== ПЛАНИРОВЩИК УЧЕБНЫХ СЕССИЙ (Ruby) ====="
    puts "1. Добавить сессию"
    puts "2. Показать все сессии"
    puts "3. Показать сессии по предмету"
    puts "4. Отметить сессию как выполненную"
    puts "5. Редактировать сессию"
    puts "6. Удалить сессию"
    puts "7. Показать статистику"
    puts "8. Сохранить в файл"
    puts "9. Загрузить из файла"
    puts "0. Выход"
    print "Выберите действие: "
    choice = gets.chomp

    case choice
    when "0"
      break
    when "1"
      print "Предмет: "
      subject = gets.chomp
      next if subject.empty?
      print "Тема: "
      topic = gets.chomp
      print "Дата и время (ГГГГ-ММ-ДД ЧЧ:ММ): "
      datetime = gets.chomp
      print "Длительность (мин): "
      duration = gets.chomp.to_i
      puts "Приоритет: 1-Низкий, 2-Средний, 3-Высокий"
      prio = gets.chomp
      priority = case prio
                 when "1" then Priority::LOW
                 when "3" then Priority::HIGH
                 else Priority::MEDIUM
                 end
      begin
        session = planner.add_session(subject, topic, datetime, duration, priority)
        puts "Сессия добавлена с ID #{session.id}"
      rescue => e
        puts "Ошибка: #{e.message}"
      end
    when "2"
      if planner.sessions.empty?
        puts "Нет сессий."
      else
        planner.sessions.each { |s| print_session(s) }
      end
    when "3"
      print "Введите предмет: "
      subject = gets.chomp
      sessions = planner.filter_by_subject(subject)
      if sessions.empty?
        puts "Сессий по этому предмету нет."
      else
        sessions.each { |s| print_session(s) }
      end
    when "4"
      print "Введите ID сессии: "
      id = gets.chomp.to_i
      if planner.set_status(id, SessionStatus::COMPLETED)
        puts "Сессия отмечена как выполненная."
      else
        puts "Сессия не найдена."
      end
    when "5"
      print "Введите ID сессии для редактирования: "
      id = gets.chomp.to_i
      session = planner.find_session(id)
      unless session
        puts "Сессия не найдена."
        next
      end
      puts "Оставьте поле пустым, чтобы не менять."
      print "Предмет (#{session.subject}): "
      new_subj = gets.chomp
      print "Тема (#{session.topic}): "
      new_topic = gets.chomp
      print "Дата/время (#{session.datetime}): "
      new_dt = gets.chomp
      print "Длительность (#{session.duration}): "
      new_dur = gets.chomp
      print "Приоритет (1-Низкий,2-Средний,3-Высокий) сейчас: #{session.priority}: "
      new_prio = gets.chomp
      updates = {}
      updates[:subject] = new_subj unless new_subj.empty?
      updates[:topic] = new_topic unless new_topic.empty?
      updates[:datetime] = new_dt unless new_dt.empty?
      unless new_dur.empty?
        updates[:duration] = new_dur.to_i
      end
      unless new_prio.empty?
        updates[:priority] = case new_prio
                             when "1" then Priority::LOW
                             when "3" then Priority::HIGH
                             else Priority::MEDIUM
                             end
      end
      if planner.update_session(id, **updates)
        puts "Сессия обновлена."
      else
        puts "Ошибка обновления."
      end
    when "6"
      print "Введите ID сессии для удаления: "
      id = gets.chomp.to_i
      if planner.delete_session(id)
        puts "Сессия удалена."
      else
        puts "Сессия не найдена."
      end
    when "7"
      stats = planner.stats
      puts "\n=== СТАТИСТИКА ==="
      puts "Всего сессий: #{stats[:total]}"
      puts "Выполнено: #{stats[:completed]}"
      puts "Запланировано: #{stats[:planned]}"
      puts "Пропущено: #{stats[:missed]}"
      puts "Общее время: #{stats[:total_time]} мин"
      puts "Время выполненных: #{stats[:completed_time]} мин"
      puts "Средняя длительность: #{'%.1f' % stats[:avg_duration]} мин"
      puts "По предметам:"
      stats[:by_subject].each { |subj, mins| puts "  #{subj}: #{mins} мин" }
    when "8"
      planner.save_to_file
      puts "Сохранено."
    when "9"
      planner.load_from_file
      puts "Загружено."
    else
      puts "Неизвестная команда."
    end
  end
end

main if __FILE__ == $0
