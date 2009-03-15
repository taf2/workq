class Job
  def initialize(row)
    if not defined?(@@class_init)
      row.keys.each { |key| Job.class_eval { attr_reader key } }
      @@class_init = true
    end
    row.keys.each { |key|  self.instance_variable_set("@#{key}", row[key]) }
  end
end

