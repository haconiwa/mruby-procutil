module Procutil
  def system4(cmd, newstdin=nil, newstdout=nil, newstderr=nil)
    newstdin  ||= 0
    newstdout ||= 1
    newstderr ||= 2
    pid, status = *__system4(cmd, to_fileno(newstdin), to_fileno(newstdout), to_fileno(newstderr))
    if Object.const_defined?(:Process) && Process.const_defined?(:Status)
      Process::Status.new(pid, status)
    else
      [pid, status]
    end
  end
  module_function :system4

  def to_fileno(fd_or_io)
    if fd_or_io.is_a?(Integer)
      fd_or_io
    else
      fd_or_io.fileno
    end
  end
  module_function :to_fileno
  private :to_fileno
end
